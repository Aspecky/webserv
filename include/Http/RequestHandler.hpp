#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "Config/ConfigTypes.hpp"
#include <string>

class HttpRequest;
class HttpResponse;

class RequestHandler {
  public:
	explicit RequestHandler(const ServerConfig &config);

	void handle(const HttpRequest &req, HttpResponse &res);

	void handleError(int status, HttpResponse &res);

  private:
	RequestHandler(const RequestHandler &);
	RequestHandler &operator=(const RequestHandler &);

	const ServerConfig &config_;

	// Request processing chunks
	const LocationConfig *processLocation_(const HttpRequest &req, HttpResponse &res, std::string &matchedPrefix);
	bool				  processMethodValidation_(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res);
	bool				  processRedirect_(const LocationConfig &loc, HttpResponse &res);
	void				  dispatchMethod_(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res, std::string &matched);

	// Routing
	const LocationConfig *matchLocation_(const std::string &uri, std::string	&matchPrefix) const;
	bool	 isMethodAllowed_(const LocationConfig &loc, const std::string	   &method) const;

	// Method dispatch
	void handleGet_(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res, const std::string &matched, bool withBody);
	void handleDirectoryGet_(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res, const std::string &fullPath, bool withBody);
	void handleFileGet_(const LocationConfig &loc, HttpResponse &res,  const std::string &fullPath, bool withBody);
	std::string resolvePath_(const std::string &root, const std::string &uri, const std::string &matched) const;

	void handleHead_(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res);
	void handlePost_(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res);
	void handleDelete_(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res, const std::string &matched);

	// POST multipart/form-data
	void handleMultipart_(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res);
	bool saveUploadedFile_(const std::string &uploadStore, const std::string &filename, const std::string &content) const;
	std::string extractMultipartFilename_(const std::string &partHeader) const;

	// Filesystem helpers
	std::string buildDirectoryListing_(const std::string &path, const std::string &url) const;
	std::string mimeTypes_(const std::string &path) const;
	std::string readFile_(const std::string &path, bool &ok) const;
	bool		isDirectory_(const std::string &path) const;
	bool		fileExists_(const std::string &path) const;

	void writeErrorBody_(int status, HttpResponse &res) const;

	static std::string decodeURI_(const std::string &s);
};

#endif
