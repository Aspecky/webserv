#include "Http/RequestHandler.hpp"
#include "Config/ConfigTypes.hpp"
#include "Http/Grammar.hpp"
#include "Http/Helper.hpp"
#include "Http/HttpRequest.hpp"
#include "Http/HttpResponse.hpp"
#include "Http/StatusCodes.hpp"
#include <cctype>
#include <cstdio>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <ios>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

RequestHandler::RequestHandler(const ServerConfig &config) : config_(config)
{
}

RequestHandler::~RequestHandler()
{
}

void RequestHandler::handle(const HttpRequest &req, HttpResponse &res)
{
	res.clear();

	std::string			  matched;
	const LocationConfig *loc = matchLocation_(req.path(), matched);

	if (!loc) {
		handleError(status_codes::NOT_FOUND, res);
		return;
	}

	if (!isMethodAllowed_(*loc, req.method())) {
		handleError(status_codes::METHOD_NOT_ALLOWED, res);
		return;
	}

	if (!loc->redirect.empty()) {
		res.setHeader("Location", loc->redirect);
		res.setStatus(status_codes::MOVED_PERMANENTLY);
		return;
	}

	if (req.method() == "GET") {
		handleGet_(req, *loc, res, matched, true);
	}
	else if (req.method() == "HEAD") {
		handleHead_(req, *loc, res);
	}
	else if (req.method() == "POST") {
		handlePost_(req, *loc, res);
	}
	else if (req.method() == "DELETE") {
		handleDelete_(req, *loc, res, matched);
	}
	else {
		handleError(status_codes::NOT_IMPLEMENTED, res);
	}
}

const LocationConfig *
RequestHandler::matchLocation_(const std::string &uri,
							   std::string		 &matchedPrefix) const
{
	const LocationConfig *loc = NULL;

	const std::map<std::string, LocationConfig> &locations = config_.locations;
	for (std::map<std::string, LocationConfig>::const_iterator it =
			 locations.begin();
		 it != locations.end(); ++it) {
		const std::string &prefix = it->first;
		if (uri.compare(0, prefix.size(), prefix) == 0 &&
			prefix.size() > matchedPrefix.size()) {
			matchedPrefix = prefix;
			loc			  = &it->second;
		}
	}
	return loc;
}

bool RequestHandler::isMethodAllowed_(const LocationConfig &loc,
									  const std::string	   &method) const
{
	std::vector<std::string> vec = loc.methods;
	for (std::size_t i = 0; i < vec.size(); ++i) {
		if (vec[i] == method) {
			return true;
		}
	}
	return false;
}

std::string RequestHandler::buildDirectoryListing_(const std::string &path,
												   const std::string &url) const
{

	DIR *dir = opendir(path.c_str());
	if (!dir) {
		return RequestHelpers::Empty;
	}

	std::cout << "------------------> " << path << " <------------\n";
	std::ostringstream body;
	body << "<html><head><title>Index of " << url << "</title></head>"
		 << "<body><h1>Index of " << url << "</h1><hr><pre>";

	struct dirent *entry = NULL;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;
		std::cout << "FILE : " << name << std::endl;
		body << "<a href=\"" << url;
		if (url.empty() || url[url.size() - 1] != '/') {
			body << '/';
		}
		body << name << "\">" << name << "</a>\n";
	}
	closedir(dir);
	body << "</pre><hr></body></html>";
	return body.str();
}

bool RequestHandler::isDirectory_(const std::string &path) const
{
	struct stat st = {};
	if (stat(path.c_str(), &st) != 0) {
		return false;
	}
	return S_ISDIR(st.st_mode);
}

std::string RequestHandler::readFile_(const std::string &path, bool &ok) const
{
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		return RequestHelpers::Empty;
	}

	std::ostringstream ss;
	ss << file.rdbuf();
	ok = true;
	return ss.str();
}

bool RequestHandler::fileExists_(const std::string &path) const
{
	struct stat st = {};
	if (stat(path.c_str(), &st) != 0) {
		return false;
	}
	return S_ISREG(st.st_mode);
}

void RequestHandler::handleError(int status, HttpResponse &res)
{
	res.clear();
	res.setStatus(status);
	writeErrorBody_(status, res);
}

void RequestHandler::writeErrorBody_(int status, HttpResponse &res) const
{
	std::map<int, std::string>::const_iterator it =
		config_.error_pages.find(status);
	if (it != config_.error_pages.end()) {
		bool		ok	 = false;
		std::string body = readFile_(it->second, ok);
		if (ok) {
			res.setHeader("content-type", mimeTypes_(it->second));
			res.setHeader("content-length",
						  RequestHelpers::sizeToString(body.size()));
			res.setBody(body);
			return;
		}
	}

	std::ostringstream html;
	html << "<html><body><h1>" << status << " " << res.statusReason()
		 << "</h1></body></html>";

	const std::string body = html.str();
	res.setHeader("content-type", "text/html");
	res.setHeader("content-length", RequestHelpers::sizeToString(body.size()));
	res.setBody(body);
}

std::string RequestHandler::mimeTypes_(const std::string &path) const
{
	std::size_t dot = path.rfind('.');
	if (dot == std::string::npos || dot < path.find_last_of('/')) {
		return "application/octet-stream";
	}

	std::string ext = path.substr(dot + 1);

	for (std::size_t i = 0; i < ext.size(); ++i) {
		ext[i] =
			static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i])));
	}

	return RequestHelpers::getContentType(ext);
}

static int hexVal(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return -1;
}

std::string RequestHandler::decodeURI_(const std::string &s)
{
	const char *pos = s.data();
	const char *end = pos + s.size();
	std::string out;

	out.reserve(s.size());

	while (pos < end) {
		const char *next = grammar::uri::PctEncoded()(pos, end);
		if (next) {
			out += static_cast<char>((hexVal(pos[1]) << 4) | hexVal(pos[2]));
			pos = next;
		}
		else {
			out += *pos++;
		}
	}
	return out;
}
