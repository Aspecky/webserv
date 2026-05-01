#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <cstddef>
#include <map>
#include <string>

// Passive value type — symmetric with HttpRequest.
//
// Owned by Client, populated by RequestHandler, read by HttpResponseWriter.
// Holds no parser/serializer state and performs no I/O.
class HttpResponse {
  public:
	HttpResponse();

	// ---- Writers (used by RequestHandler) ----------------------------------

	// Set the status code; reason phrase is looked up from a default table.
	void setStatus(int code);
	void setStatus(int code, const std::string &reason);

	// Header keys are stored lowercase (matches HttpRequest convention).
	// setHeader overwrites; if multi-valued headers are ever needed
	// (e.g. Set-Cookie), add an addHeader() then.
	void setHeader(const std::string &name, const std::string &value);

	void setBody(const std::string &body);
	void appendBody(const char *data, std::size_t n);

	// ---- Readers (used by HttpResponseWriter and tests) --------------------

	int statusCode() const;
	const std::string &statusReason() const;

	const std::map<std::string, std::string> &headers() const;
	bool hasHeader(const std::string &name) const;
	const std::string &header(const std::string &name) const;

	const std::string &body() const;

	// Reset for the next exchange on a keep-alive connection.
	void clear();

  private:
	HttpResponse(const HttpResponse &);
	HttpResponse &operator=(const HttpResponse &);

	int								   statusCode_;
	std::string						   statusReason_;
	std::map<std::string, std::string> headers_;
	std::string						   body_;
};

#endif
