#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "Config/ConfigTypes.hpp"
#include <string>

class HttpRequest;
class HttpResponse;

class RequestHandler {
  public:
	explicit RequestHandler(const ServerConfig &confg);

	void handle(const HttpRequest &req, HttpResponse &res);

	void handleError(int status, HttpResponse &res);

  private:
	RequestHandler(const RequestHandler &);
	RequestHandler &operator=(const RequestHandler &);

	const ServerConfig &config_;

	// Routing
	const LocationConfig *matchLocation_(const std::string &uri,
										std::string		  &matchPrefix) const;
	bool				  isMethodAllowed_(const LocationConfig &loc,
										  const std::string	   &method) const;

	// Method dispatch
	void handleGet_(const HttpRequest &req, const LocationConfig &loc,
				   HttpResponse &res, std::string &matched, bool withBody);
	void handleHead_(const HttpRequest &req, const LocationConfig &loc,
					HttpResponse &res);
	void handlePost_(const HttpRequest &req, const LocationConfig &loc,
					HttpResponse &res);
	void handleDelete_(const HttpRequest &req, const LocationConfig &loc,
					  HttpResponse &res, const std::string &matched);

	// POST multipart/form-data
	void handleMultipart_(const HttpRequest &req, const LocationConfig &loc,
						 HttpResponse &res);

	// Filesystem helpers
	std::string buildDirectoryListing_(const std::string &path,
									  const std::string &url) const;
	std::string mimeTypes_(const std::string &path) const;
	std::string readFile_(const std::string &path, bool &ok) const;
	bool		isDirectory_(const std::string &path) const;
	bool		fileExists_(const std::string &path) const;

	void writeErrorBody_(int status, HttpResponse &res) const;
};

#endif
