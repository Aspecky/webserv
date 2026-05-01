#include "RequestHandler.hpp"

#include "HttpResponse.hpp"
#include "Http/HttpRequest.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace {

std::string sizeToString(std::size_t n)
{
	std::ostringstream ss;
	ss << n;
	return ss.str();
}

std::string toLower(const std::string &s)
{
	std::string out(s);
	for (std::size_t i = 0; i < out.size(); ++i)
		out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
	return out;
}

// Multipart helpers — used only by handlePost.

std::string extractBoundary(const std::string &contentType)
{
	const std::size_t pos = contentType.find("boundary=");
	if (pos == std::string::npos)
		return "";
	std::string b = contentType.substr(pos + 9);
	if (!b.empty() && b[0] == '"')
		b = b.substr(1, b.size() - 2);
	return b;
}

std::string extractFilename(const std::string &disposition)
{
	const std::size_t pos = disposition.find("filename=\"");
	if (pos == std::string::npos)
		return "";
	const std::size_t start = pos + 10;
	const std::size_t end	= disposition.find('"', start);
	if (end == std::string::npos)
		return "";
	return disposition.substr(start, end - start);
}

std::string getDisposition(const std::string &partHeaders)
{
	const std::string lower = toLower(partHeaders);
	const std::size_t pos	= lower.find("content-disposition:");
	if (pos == std::string::npos)
		return "";
	const std::size_t start = pos + 20;
	const std::size_t end	= lower.find("\r\n", start);
	std::string		  value =
		partHeaders.substr(start,
						   (end == std::string::npos ? partHeaders.size() : end) - start);
	const std::size_t s = value.find_first_not_of(" \t");
	return (s == std::string::npos) ? "" : value.substr(s);
}

struct MimeRow {
	const char *ext;
	const char *type;
};

const MimeRow kMimeTable[] = {
	{"html", "text/html"},		   {"htm", "text/html"},
	{"css", "text/css"},		   {"txt", "text/plain"},
	{"xml", "text/xml"},		   {"js", "application/javascript"},
	{"json", "application/json"},  {"pdf", "application/pdf"},
	{"zip", "application/zip"},	   {"png", "image/png"},
	{"gif", "image/gif"},		   {"jpg", "image/jpeg"},
	{"jpeg", "image/jpeg"},		   {"webp", "image/webp"},
	{"svg", "image/svg+xml"},	   {"ico", "image/x-icon"},
	{"mp3", "audio/mpeg"},		   {"mp4", "video/mp4"},
	{"webm", "video/webm"},		   {"woff", "font/woff"},
	{"woff2", "font/woff2"},
};

} // namespace

RequestHandler::RequestHandler(const ServerConfig &cfg)
	: cfg_(cfg)
{
}

// ---- public API -----------------------------------------------------------

void RequestHandler::handle(const HttpRequest &req, HttpResponse &res)
{
	res.clear();

	std::string			  matched;
	const LocationConfig *loc = matchLocation(req.uri(), matched);
	if (!loc) {
		handleError(404, res);
		return;
	}

	if (!isMethodAllowed(*loc, req.method())) {
		handleError(405, res);
		return;
	}

	if (!loc->redirect.empty()) {
		res.setStatus(301);
		res.setHeader("Location", loc->redirect);
		return;
	}

	if (req.method() == "GET")
		handleGet(req, *loc, res, true);
	else if (req.method() == "HEAD")
		handleHead(req, *loc, res);
	else if (req.method() == "POST")
		handlePost(req, *loc, res);
	else if (req.method() == "DELETE")
		handleDelete(req, *loc, res);
	else
		handleError(501, res);
}

void RequestHandler::handleError(int status, HttpResponse &res)
{
	res.clear();
	res.setStatus(status);
	writeErrorBody(status, res);
}

// ---- routing --------------------------------------------------------------

const LocationConfig *
RequestHandler::matchLocation(const std::string &uri,
							  std::string		&matchedPrefix) const
{
	const LocationConfig *best = NULL;
	matchedPrefix.clear();

	const std::map<std::string, LocationConfig> &locations = cfg_.locations;
	for (std::map<std::string, LocationConfig>::const_iterator it =
			 locations.begin();
		 it != locations.end(); ++it) {
		const std::string &prefix = it->first;
		if (uri.compare(0, prefix.size(), prefix) == 0 &&
			prefix.size() > matchedPrefix.size()) {
			matchedPrefix = prefix;
			best		  = &it->second;
		}
	}
	return best;
}

bool RequestHandler::isMethodAllowed(const LocationConfig &loc,
									 const std::string	  &method) const
{
	for (std::size_t i = 0; i < loc.methods.size(); ++i) {
		if (loc.methods[i] == method)
			return true;
	}
	return false;
}

// ---- GET / HEAD -----------------------------------------------------------

void RequestHandler::handleGet(const HttpRequest	&req,
							   const LocationConfig &loc,
							   HttpResponse			&res,
							   bool					 withBody)
{
	std::string fullPath = loc.root;

	if (isDirectory(fullPath)) {
		std::string indexPath = fullPath;
		if (indexPath.empty() || indexPath[indexPath.size() - 1] != '/')
			indexPath += '/';
		indexPath += loc.index.empty() ? "index.html" : loc.index;

		if (fileExists(indexPath)) {
			bool		ok	 = false;
			std::string body = readFile(indexPath, ok);
			if (!ok) {
				handleError(500, res);
				return;
			}
			res.setStatus(200);
			res.setHeader("Content-Type", mimeTypeFor(indexPath));
			res.setHeader("Content-Length", sizeToString(body.size()));
			if (withBody)
				res.setBody(body);
			return;
		}

		if (loc.directory_listing) {
			const std::string body = buildDirectoryListing(fullPath, req.uri());
			res.setStatus(200);
			res.setHeader("Content-Type", "text/html");
			res.setHeader("Content-Length", sizeToString(body.size()));
			if (withBody)
				res.setBody(body);
			return;
		}

		handleError(403, res);
		return;
	}

	if (!fileExists(fullPath)) {
		handleError(404, res);
		return;
	}

	bool		ok	 = false;
	std::string body = readFile(fullPath, ok);
	if (!ok) {
		handleError(500, res);
		return;
	}
	res.setStatus(200);
	res.setHeader("Content-Type", mimeTypeFor(fullPath));
	res.setHeader("Content-Length", sizeToString(body.size()));
	if (withBody)
		res.setBody(body);
}

void RequestHandler::handleHead(const HttpRequest	 &req,
								const LocationConfig &loc,
								HttpResponse		 &res)
{
	handleGet(req, loc, res, false);
}

// ---- POST -----------------------------------------------------------------

void RequestHandler::handlePost(const HttpRequest	 &req,
								const LocationConfig &loc,
								HttpResponse		 &res)
{
	const std::string &contentType = req.header("content-type");
	const std::string &body		   = req.body();

	if (contentType.find("multipart/form-data") != std::string::npos) {
		if (loc.upload_store.empty()) {
			handleError(400, res);
			return;
		}

		const std::string boundary = extractBoundary(contentType);
		if (boundary.empty()) {
			handleError(400, res);
			return;
		}

		const std::string delimiter	   = "--" + boundary;
		int				  filesUploaded = 0;
		std::size_t		  pos		   = 0;

		while (true) {
			const std::size_t boundaryPos = body.find(delimiter, pos);
			if (boundaryPos == std::string::npos)
				break;
			pos = boundaryPos + delimiter.size();

			if (body.compare(pos, 2, "--") == 0)
				break;

			if (body.compare(pos, 2, "\r\n") == 0)
				pos += 2;

			const std::size_t headerEnd = body.find("\r\n\r\n", pos);
			if (headerEnd == std::string::npos)
				break;

			const std::string partHeaders = body.substr(pos, headerEnd - pos);
			pos							  = headerEnd + 4;

			const std::size_t bodyEnd = body.find("\r\n" + delimiter, pos);
			if (bodyEnd == std::string::npos)
				break;
			const std::string partBody = body.substr(pos, bodyEnd - pos);
			pos						   = bodyEnd + 2;

			std::string filename = extractFilename(getDisposition(partHeaders));
			if (filename.empty())
				continue;
			const std::size_t slash = filename.rfind('/');
			if (slash != std::string::npos)
				filename = filename.substr(slash + 1);
			if (filename.empty())
				filename = "upload_" + sizeToString(static_cast<std::size_t>(time(NULL)));

			const std::string dest = loc.upload_store + "/" + filename;
			std::ofstream	  out(dest.c_str(), std::ios::binary);
			if (!out.is_open()) {
				handleError(500, res);
				return;
			}
			out.write(partBody.c_str(), static_cast<std::streamsize>(partBody.size()));
			out.close();
			++filesUploaded;
		}

		if (filesUploaded == 0) {
			handleError(400, res);
			return;
		}
		const std::string ok = "Files uploaded successfully.";
		res.setStatus(201);
		res.setHeader("Content-Type", "text/plain");
		res.setHeader("Content-Length", sizeToString(ok.size()));
		res.setBody(ok);
		return;
	}

	if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
		if (body.empty()) {
			handleError(400, res);
			return;
		}
		std::ostringstream html;
		html << "<html><body><h2>Form Received</h2><ul>";
		std::string remaining = body;
		while (!remaining.empty()) {
			const std::size_t amp = remaining.find('&');
			const std::string pair = (amp == std::string::npos) ? remaining : remaining.substr(0, amp);

			remaining = (amp == std::string::npos) ? "" : remaining.substr(amp + 1);
			const std::size_t eq = pair.find('=');
			if (eq != std::string::npos) {
				html << "<li><b>" << pair.substr(0, eq) << "</b>: "
					 << pair.substr(eq + 1) << "</li>";
			}
		}
		html << "</ul></body></html>";
		const std::string out = html.str();
		res.setStatus(200);
		res.setHeader("Content-Type", "text/html");
		res.setHeader("Content-Length", sizeToString(out.size()));
		res.setBody(out);
		return;
	}

	// Raw body fallback (e.g. application/octet-stream, application/json).
	if (loc.upload_store.empty()) {
		handleError(400, res);
		return;
	}
	const std::string dest =
		loc.upload_store + "/upload_" + sizeToString(static_cast<std::size_t>(time(NULL)));
	std::ofstream out(dest.c_str(), std::ios::binary);
	if (!out.is_open()) {
		handleError(500, res);
		return;
	}
	out.write(body.c_str(), static_cast<std::streamsize>(body.size()));
	out.close();

	const std::string ok = "Uploaded successfully.";
	res.setStatus(201);
	res.setHeader("Content-Type", "text/plain");
	res.setHeader("Content-Length", sizeToString(ok.size()));
	res.setBody(ok);
}

// ---- DELETE ---------------------------------------------------------------

void RequestHandler::handleDelete(const HttpRequest	   &req,
								  const LocationConfig &loc,
								  HttpResponse		   &res)
{
	const std::string relativePath = req.uri().substr(loc.path.size());
	const std::string fullPath	   = loc.root + relativePath;

	if (!fileExists(fullPath)) {
		handleError(404, res);
		return;
	}

	if (std::remove(fullPath.c_str()) != 0) {
		handleError(500, res);
		return;
	}
	const std::string body = "File deleted.";
	res.setStatus(200);
	res.setHeader("Content-Type", "text/plain");
	res.setHeader("Content-Length", sizeToString(body.size()));
	res.setBody(body);
}

// ---- filesystem helpers ---------------------------------------------------

std::string RequestHandler::buildDirectoryListing(const std::string &path,
												  const std::string &url) const
{
	DIR *dir = opendir(path.c_str());
	if (!dir)
		return "";

	std::ostringstream body;
	body << "<html><head><title>Index of " << url << "</title></head>"
		 << "<body><h1>Index of " << url << "</h1><hr><pre>";

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		const std::string name = entry->d_name;
		body << "<a href=\"" << url;
		if (url.empty() || url[url.size() - 1] != '/')
			body << '/';
		body << name << "\">" << name << "</a>\n";
	}
	closedir(dir);
	body << "</pre><hr></body></html>";
	return body.str();
}

std::string RequestHandler::readFile(const std::string &path, bool &ok) const
{
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		ok = false;
		return "";
	}
	std::ostringstream ss;
	ss << file.rdbuf();
	ok = true;
	return ss.str();
}

std::string RequestHandler::mimeTypeFor(const std::string &path) const
{
	const std::size_t dot = path.rfind('.');
	if (dot == std::string::npos)
		return "application/octet-stream";

	std::string ext = path.substr(dot + 1);
	for (std::size_t i = 0; i < ext.size(); ++i)
		ext[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i])));

	const std::size_t n = sizeof(kMimeTable) / sizeof(kMimeTable[0]);
	for (std::size_t i = 0; i < n; ++i) {
		if (ext == kMimeTable[i].ext)
			return kMimeTable[i].type;
	}
	return "application/octet-stream";
}

bool RequestHandler::isDirectory(const std::string &path) const
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;
	return S_ISDIR(st.st_mode);
}

bool RequestHandler::fileExists(const std::string &path) const
{
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}

void RequestHandler::writeErrorBody(int code, HttpResponse &res) const
{
	// If the server config has a custom error page, prefer it.
	const std::map<int, std::string>::const_iterator it = cfg_.error_pages.find(code);
	if (it != cfg_.error_pages.end()) {
		bool		ok	 = false;
		std::string body = readFile(it->second, ok);
		if (ok) {
			res.setHeader("Content-Type", mimeTypeFor(it->second));
			res.setHeader("Content-Length", sizeToString(body.size()));
			res.setBody(body);
			return;
		}
	}

	std::ostringstream html;
	html << "<html><body><h1>" << code << " " << res.statusReason()
		 << "</h1></body></html>";
	const std::string body = html.str();
	res.setHeader("Content-Type", "text/html");
	res.setHeader("Content-Length", sizeToString(body.size()));
	res.setBody(body);
}
