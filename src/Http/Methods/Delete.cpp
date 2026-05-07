#include "Http/Helper.hpp"
#include "Http/HttpRequest.hpp"
#include "Http/HttpResponse.hpp"
#include "Http/RequestHandler.hpp"
#include "Http/StatusCodes.hpp"

#include <cstdio>
#include <iostream>

void RequestHandler::handleDelete(const HttpRequest	   &req,
								  const LocationConfig &loc, HttpResponse &res,
								  const std::string &matched)
{
	std::string relativePath =
		RequestHelpers::urlDecode(req.uri().substr(matched.size()));

	std::string fullPath = loc.root;
	if (!fullPath.empty() && fullPath[fullPath.size() - 1] != '/') {
		if (relativePath.empty() || relativePath[0] != '/')
			fullPath += '/';
	}
	fullPath += relativePath;

	std::cout << "[ DELETE " << fullPath << " ]" << std::endl;

	if (isDirectory(fullPath)) {
		handleError(status_codes::FORBIDDEN, res);
		return;
	}

	if (!fileExists(fullPath)) {
		handleError(status_codes::NOT_FOUND, res);
		return;
	}

	if (std::remove(fullPath.c_str()) != 0) {
		handleError(status_codes::INTERNAL_SERVER_ERROR, res);
		return;
	}

	const std::string body = "File deleted.";
	res.setStatus(status_codes::OK);
	res.setHeader("Content-Type", "text/plain");
	res.setHeader("Content-Length", RequestHelpers::sizeToString(body.size()));
	res.setBody(body);
}
