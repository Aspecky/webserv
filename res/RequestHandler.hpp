#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include "Config/ConfigTypes.hpp"

#include <string>

class HttpRequest;
class HttpResponse;

// Application layer.
//
// Translates a parsed request + the matching server config into a populated
// HttpResponse. The only HTTP component that may touch the filesystem or
// fork/exec. Synchronous for static content; CGI will extend this later.
class RequestHandler {
  public:
	explicit RequestHandler(const ServerConfig &cfg);

	// Build a response for a successfully parsed request.
	void handle(const HttpRequest &req, HttpResponse &res);

	// Build an error response for a parser/transport failure. Centralizing
	// error rendering here keeps "what does a 4xx/5xx look like" in one place.
	void handleError(int status, HttpResponse &res);

  private:
	RequestHandler(const RequestHandler &);
	RequestHandler &operator=(const RequestHandler &);

	const ServerConfig &cfg_;

	// Routing
	const LocationConfig *matchLocation(const std::string &uri,
										std::string		   &matchedPrefix) const;
	bool				  isMethodAllowed(const LocationConfig &loc,
										  const std::string	   &method) const;

	// Method dispatch
	void handleGet(const HttpRequest	&req,
				   const LocationConfig &loc,
				   HttpResponse			&res,
				   bool					 withBody);
	void handleHead(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res);
	void handlePost(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res);
	void handleDelete(const HttpRequest	  &req,
					  const LocationConfig &loc,
					  HttpResponse		   &res);

	// Filesystem helpers
	std::string buildDirectoryListing(const std::string &path,
									  const std::string &url) const;
	std::string readFile(const std::string &path, bool &ok) const;
	std::string mimeTypeFor(const std::string &path) const;
	bool		isDirectory(const std::string &path) const;
	bool		fileExists(const std::string &path) const;

	// Error body rendering
	void writeErrorBody(int code, HttpResponse &res) const;
};

#endif
