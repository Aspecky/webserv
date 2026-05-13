#include "Http/RequestHandler.hpp"
#include "Config/ConfigTypes.hpp"
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

static int hexVal_(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

std::string RequestHandler::decodeURI_(const std::string &s)
{
	std::string out;
	out.reserve(s.size());
	for (std::size_t i = 0; i < s.size(); ++i) {
		if (s[i] == '%' && i + 2 < s.size()) {
			int hi = hexVal_(s[i + 1]);
			int lo = hexVal_(s[i + 2]);
			if (hi >= 0 && lo >= 0) {
				out += static_cast<char>((hi << 4) | lo);
				i += 2;
				continue;
			}
		}
		if (s[i] == '+') {
			out += ' ';
		}
		else {
			out += s[i];
		}
	}
	return out;
}

RequestHandler::RequestHandler(const ServerConfig &config) : config_(config)
{
}

// void printServerConfig(const ServerConfig& server);

void RequestHandler::handle(const HttpRequest &req, HttpResponse &res)
{
	res.clear();

	std::string			  matched;
	const LocationConfig *loc = processLocation_(req, res, matched);
	if (!loc) {
		return;
	}

	if (!processMethodValidation_(req, *loc, res)) {
		return;
	}

	if (processRedirect_(*loc, res)) {
		return;
	}

	dispatchMethod_(req, *loc, res, matched);
}

const LocationConfig *
RequestHandler::processLocation_(const HttpRequest &req, HttpResponse &res,
								 std::string &matchedPrefix)
{
	std::cout << "URI -----> " << req.path() << std::endl;

	const LocationConfig *loc = matchLocation_(req.path(), matchedPrefix);
	if (!loc) {
		handleError(status_codes::NOT_FOUND, res);
		return NULL;
	}

	std::cout << "Matched path is ---->" << matchedPrefix << std::endl;
	return loc;
}

bool RequestHandler::processMethodValidation_(const HttpRequest	   &req,
											  const LocationConfig &loc,
											  HttpResponse		   &res)
{
	if (!isMethodAllowed_(loc, req.method())) {
		handleError(status_codes::METHOD_NOT_ALLOWED, res);
		return false;
	}
	return true;
}

bool RequestHandler::processRedirect_(const LocationConfig &loc,
									  HttpResponse		   &res)
{
	if (!loc.redirect.empty()) {
		res.setStatus(status_codes::MOVED_PERMANENTLY);
		res.setHeader("Location", loc.redirect);
		return true;
	}
	return false;
}

void RequestHandler::dispatchMethod_(const HttpRequest	  &req,
									 const LocationConfig &loc,
									 HttpResponse &res, std::string &matched)
{
	std::cout << "method -----> " << req.method() << std::endl;
	if (req.method() == "GET") {
		handleGet_(req, loc, res, matched, true);
	}
	else if (req.method() == "HEAD") {
		handleHead_(req, loc, res);
	}
	else if (req.method() == "POST") {
		handlePost_(req, loc, res);
	}
	else if (req.method() == "DELETE") {
		handleDelete_(req, loc, res, matched);
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
	matchedPrefix.clear();

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

void RequestHandler::writeErrorBody_(int code, HttpResponse &res) const
{
	const std::map<int, std::string>::const_iterator it =
		config_.error_pages.find(code);
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
	html << "<html><body><h1>" << code << " " << res.statusReason()
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
