#include "Config/ConfigTypes.hpp"
#include "Http/Helper.hpp"
#include "Http/HttpRequest.hpp"
#include "Http/HttpResponse.hpp"
#include "Http/RequestHandler.hpp"
#include "Http/StatusCodes.hpp"
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

void RequestHandler::handlePost(const HttpRequest	 &req,
								const LocationConfig &loc, HttpResponse &res)
{
	const std::string &contentType = req.header("content-type");

	if (contentType.find("multipart/form-data") != std::string::npos) {
		handleMultipart(req, loc, res);
		return;
	}

	handleError(status_codes::UNSUPPORTED_MEDIA_TYPE, res); // Unsupported Media Type
}

void RequestHandler::handleMultipart(const HttpRequest	  &req,
									 const LocationConfig &loc,
									 HttpResponse		  &res)
{
	const std::string &contentType = req.header("content-type");
	const std::string &body		   = req.body();

	if (loc.upload_store.empty()) {
		handleError(status_codes::BAD_REQUEST, res);
		return;
	}

	std::string boundary = RequestHelpers::extractBoundary(contentType);

	if (boundary.empty()) {
		handleError(status_codes::BAD_REQUEST, res);
		return;
	}

	std::string dlm			  = "--" + boundary;
	int			filesUploaded = 0;
	std::size_t pos			  = 0;

	while (true) {
		// this order from rfc 2046
		std::size_t bdPos = body.find(dlm, pos);
		if (bdPos == std::string::npos) {
			break;
		}
		pos = bdPos + dlm.size();

		if (body.compare(pos, 2, "--") == 0) {
			break;
		}

		if (body.compare(pos, 2, "\r\n") == 0) {
			pos += 2;
		}

		std::size_t endHeader = body.find("\r\n\r\n", pos);
		if (endHeader == std::string::npos) {
			break;
		}

		std::string partHeader = body.substr(pos, endHeader - pos);
		pos					   = endHeader + 4;

		std::size_t endBody = body.find("\r\n" + dlm, pos);
		if (endBody == std::string::npos) {
			break;
		}

		std::string partBody = body.substr(pos, endBody - pos);
		pos					 = endBody + 2;

		std::string filename = RequestHelpers::extractFilename(
			RequestHelpers::getDisposition(partHeader));
		if (filename.empty()) {
			continue;
		}

		std::size_t slash = filename.rfind('/');
		if (slash != std::string::npos) {
			filename = filename.substr(slash + 1);
		}
		std::size_t back = filename.rfind('\\');
		if (back != std::string::npos) {
			filename = filename.substr(back + 1);
		}
		if (filename.empty()) {
			filename = "upload_" + RequestHelpers::sizeToString(
									   static_cast<std::size_t>(time(NULL)));
		}

		std::string dest = loc.upload_store + "/" + filename;

		std::ofstream out(dest.c_str(), std::ios::binary);
		if (!out.is_open()) {
			handleError(status_codes::INTERNAL_SERVER_ERROR, res);
			return;
		}

		out.write(partBody.c_str(),
				  static_cast<std::streamsize>(partBody.size()));
		out.close();
		++filesUploaded;
	}

	if (filesUploaded == 0) {
		handleError(status_codes::BAD_REQUEST, res);
		return;
	}

	std::string ok = "Files uploaded successfully.";
	res.setStatus(status_codes::CREATED);
	res.setHeader("Content-Type", "text/plain");
	res.setHeader("Content-Length", RequestHelpers::sizeToString(ok.size()));
	res.setBody(ok);
}
