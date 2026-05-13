#include "Http/Helper.hpp"
#include "Http/HttpRequest.hpp"
#include "Http/HttpResponse.hpp"
#include "Http/RequestHandler.hpp"
#include "Http/StatusCodes.hpp"

#include <iostream>


void RequestHandler::handleGet_(const HttpRequest	&req,
							   const LocationConfig &loc, HttpResponse &res,
							   const std::string &matched, bool withBody)
{
	std::string fullPath = resolvePath_(loc.root, req.path(), matched);
	std::cout << "[ " << fullPath << " ]" << std::endl;

	if (isDirectory_(fullPath)) {
		handleDirectoryGet_(req, loc, res, fullPath, withBody);
		return;
	}

	handleFileGet_(loc, res, fullPath, withBody);
}


std::string RequestHandler::resolvePath_(const std::string &root, const std::string &uri, const std::string &matched) const
{
	std::string relativePath = uri.substr(matched.size());

	std::string fullPath = root;
	if (!fullPath.empty() && fullPath[fullPath.size() - 1] != '/') {
		if (relativePath.empty() || relativePath[0] != '/') {
			fullPath += '/';
		}
	}
	fullPath += relativePath;
	return fullPath;
}

void RequestHandler::handleDirectoryGet_(const HttpRequest &req, const LocationConfig &loc,
										 HttpResponse &res, const std::string &fullPath, bool withBody)
{
	std::string indexPath = fullPath;
	if (fullPath.empty() || indexPath[indexPath.size() - 1] != '/')
		indexPath += '/';
	indexPath += loc.index.empty() ? "index.html" : loc.index;

	std::cout << "Full path --------> " << indexPath << std::endl;
	if (fileExists_(indexPath)) {
		bool		ok	 = false;
		std::string body = readFile_(indexPath, ok);
		if (!ok) {
			std::cout << "file do not opend\n";
			handleError(status_codes::INTERNAL_SERVER_ERROR, res);
			return;
		}
		res.setStatus(status_codes::OK);
		res.setHeader("content-type", mimeTypes_(indexPath));
		res.setHeader("content-length",
					  RequestHelpers::sizeToString(body.size()));
		if (withBody)
			res.setBody(body);
		return;
	}
	if (loc.directory_listing) {
		std::cout << "[ Directory listing ]\n";
		std::string body = buildDirectoryListing_(fullPath, req.path());
		res.setStatus(status_codes::OK);
		res.setHeader("content-type", "text/html");
		res.setHeader("content-length",
					  RequestHelpers::sizeToString(body.size()));
		if (withBody)
			res.setBody(body);
		return;
	}

	handleError(status_codes::FORBIDDEN, res);
}

void RequestHandler::handleFileGet_(const LocationConfig &loc, HttpResponse &res, 
									const std::string &fullPath, bool withBody)
{
	(void)loc;
	if (!fileExists_(fullPath)) {
		handleError(status_codes::NOT_FOUND, res);
		return;
	}

	bool ok = false;
	std::string body = readFile_(fullPath, ok);
	if(!ok)
	{
		handleError(status_codes::INTERNAL_SERVER_ERROR, res);
		return ;
	}
	res.setStatus(status_codes::OK);
	res.setHeader("content-type", mimeTypes_(fullPath));
	res.setHeader("content-length", RequestHelpers::sizeToString(body.size()));
	if (withBody)
		res.setBody(body);
}



void RequestHandler::handleHead_(const HttpRequest	 &req,
								const LocationConfig &loc, HttpResponse &res)
{
	handleGet_(req, loc, res, const_cast<std::string &>(RequestHelpers::Empty),
			   false);
}
