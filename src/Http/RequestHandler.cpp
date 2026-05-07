#include "Http/RequestHandler.hpp"

#include "Config/ConfigTypes.hpp"

#include "Http/HttpRequest.hpp"
#include "Http/Helper.hpp"
#include "Http/HttpResponse.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
// colore

#define RESET "\033[0m"
#define BLACK "\033[30m"			  /* Black */
#define RED "\033[31m"				  /* Red */
#define GREEN "\033[32m"			  /* Green */
#define YELLOW "\033[33m"			  /* Yellow */
#define BLUE "\033[34m"				  /* Blue */
#define MAGENTA "\033[35m"			  /* Magenta */
#define CYAN "\033[36m"				  /* Cyan */
#define WHITE "\033[37m"			  /* White */
#define BOLDBLACK "\033[1m\033[30m"	  /* Bold Black */
#define BOLDRED "\033[1m\033[31m"	  /* Bold Red */
#define BOLDGREEN "\033[1m\033[32m"	  /* Bold Green */
#define BOLDYELLOW "\033[1m\033[33m"  /* Bold Yellow */
#define BOLDBLUE "\033[1m\033[34m"	  /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define BOLDCYAN "\033[1m\033[36m"	  /* Bold Cyan */
#define BOLDWHITE "\033[1m\033[37m"

RequestHandler::RequestHandler(const ServerConfig &config) : config_(config)
{
}

// void printServerConfig(const ServerConfig& server);

void RequestHandler::handle(const HttpRequest &req, HttpResponse &res)
{
	res.clear();

	// printServerConfig(config_);
	// pause();

	std::string matched;
	std::cout << "URI -----> " << req.uri() << std::endl;

	const LocationConfig *loc = matchLocation(req.uri(), matched);
	if (!loc) {
		handleError(404, res);
		return;
	}

	std::cout << "Matched path is ---->" << matched << std::endl;

	if (!isMethodAllowed(*loc, req.method())) {
		handleError(405, res);
		return;
	}

	if (!loc->redirect.empty()) {
		res.setStatus(301);
		res.setHeader("Location", loc->redirect);
		return;
	}

	std::cout << "method -----> " << req.method() << std::endl;
	if (req.method() == "GET")
		handleGet(req, *loc, res, matched, true);
	else if (req.method() == "HEAD")
		handleHead(req, *loc, res);
	else if (req.method() == "POST")
		handlePost(req, *loc, res);
	else if (req.method() == "DELETE")
		handleDelete(req, *loc, res, matched);
	else {
		handleError(501, res);
	}
}

const LocationConfig *
RequestHandler::matchLocation(const std::string &uri,
							  std::string		&matchedPrefix) const
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

bool RequestHandler::isMethodAllowed(const LocationConfig &loc,
									 const std::string	  &method) const
{
	std::vector<std::string> vec = loc.methods;
	for (std::size_t i = 0; i < vec.size(); ++i) {
		if (vec[i] == method)
			return true;
	}
	return false;
}

std::string RequestHandler::buildDirectoryListing(const std::string &path,
												  const std::string &url) const
{

	DIR *dir = opendir(path.c_str());
	if (!dir)
		return RequestHelpers::Empty;

	std::cout << "------------------> " << path << " <------------\n";
	std::ostringstream body;
	body << "<html><head><title>Index of " << url << "</title></head>"
		 << "<body><h1>Index of " << url << "</h1><hr><pre>";

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		const std::string name = entry->d_name;
		std::cout << "FILE : " << name << std::endl;
		body << "<a href=\"" << url;
		if (url.empty() || url[url.size() - 1] != '/')
			body << '/';
		body << name << "\">" << name << "</a>\n";
	}
	closedir(dir);
	body << "</pre><hr></body></html>";
	return body.str();
}

bool RequestHandler::isDirectory(const std::string &path) const
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;
	return S_ISDIR(st.st_mode);
}

std::string RequestHandler::readFile(const std::string &path, bool &ok) const
{
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file.is_open())
		return RequestHelpers::Empty;

	std::ostringstream ss;
	ss << file.rdbuf();
	ok = true;
	return ss.str();
}

bool RequestHandler::fileExists(const std::string &path) const
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;
	return S_ISREG(st.st_mode);
}

void RequestHandler::handleError(int status, HttpResponse &res)
{
	res.clear();
	res.setStatus(status);
	writeErrorBody(status, res);
}

void RequestHandler::writeErrorBody(int code, HttpResponse &res) const
{
	const std::map<int, std::string>::const_iterator it =
		config_.error_pages.find(code);
	if (it != config_.error_pages.end()) {
		bool		ok	 = false;
		std::string body = readFile(it->second, ok);
		if (ok) {
			res.setHeader("content-type", mimeTypes(it->second));
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

std::string RequestHandler::mimeTypes(const std::string &path) const
{
	const std::size_t dot = path.rfind('.');
	if (dot == std::string::npos || dot < path.find_last_of('/'))
		return "application/octet-stream";

	std::string ext = path.substr(dot + 1);

	for (std::size_t i = 0; i < ext.size(); ++i)
		ext[i] =
			static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i])));

	return RequestHelpers::getContentType(ext);
}
