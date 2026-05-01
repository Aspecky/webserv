// #include "Response/HttpResponse.hpp"
// #include <dirent.h>
// #include <fstream>
// #include <iostream>
// #include <sstream>
// #include <sys/stat.h>

// // Static member initialization

// HttpResponse::HttpResponse()
// 	: _statusCode(200), _statusText("OK"), _reqWithBody(true)
// {
// }

// void HttpResponse::buildResponse(const HttpRequest	&req,
// 								 const ServerConfig &serConf)
// {
// 	_headers.clear();
// 	_body.clear();
// 	_statusCode = 200;
// 	_statusText = _statusMsg(200);
// 	// TODO: Recheck
// 	// if (req.shouldClose())
// 	//   _headers["Connection"] = "close";
// 	// else {
// 	//   _headers["Connection"] = "keep-alive";
// 	//   _headers["Keep-Alive"] = "timeout=10, max=100";
// 	// }

// 	const LocationConfig *loc = NULL;
// 	std::string			  matchedPath;

// 	// check method
// 	const std::map<std::string, LocationConfig> &location = serConf.locations;
// 	for (std::map<std::string, LocationConfig>::const_iterator it =
// 			 location.begin();
// 		 it != location.end(); ++it) {
// 		const std::string &locPath = it->first;
// 		if (req.uri().substr(0, locPath.size()) == locPath) {
// 			if (locPath.size() > matchedPath.size()) {
// 				matchedPath = locPath;
// 				loc			= &it->second;
// 			}
// 		}
// 	}

// 	if (!loc) {
// 		_buildError(404);
// 		return;
// 	}

// 	// check if method allowed
// 	bool							allowed = false;
// 	const std::vector<std::string> &methods = loc->methods;
// 	for (size_t i = 0; i < methods.size(); i++) {
// 		if (methods[i] == req.method()) {
// 			allowed = true;
// 			break;
// 		}
// 	}

// 	if (!allowed) {
// 		_buildError(405);
// 		return;
// 	}

// 	// redirect
// 	if (!loc->redirect.empty()) {
// 		_statusCode			 = 301;
// 		_statusText			 = _statusMsg(301);
// 		_headers["Location"] = loc->redirect;
// 		return;
// 	}

// 	if (req.method() == "GET")
// 		_handleGet(req, *loc);
// 	else if (req.method() == "POST")
// 		_handlePost(req, *loc);
// 	else if (req.method() == "DELETE")
// 		_handleDelete(req, *loc);
// 	else if (req.method() == "HEAD")
// 		_handleHead(req, *loc);
// }