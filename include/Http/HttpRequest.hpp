#pragma once

#include <map>
#include <string>

class HttpParser;

class HttpRequest {
  public:
	HttpRequest();
	~HttpRequest();

	const std::string &method() const;
	const std::string &uri() const;
	const std::string &version() const;
	const std::string &body() const;

	bool			   hasHeader(const std::string &name) const;
	const std::string &header(const std::string &name) const;
	const std::map<std::string, std::string> &headers() const;

  private:
	HttpRequest(const HttpRequest &);
	HttpRequest &operator=(const HttpRequest &);

	friend class HttpParser;

	std::string						   method_;
	std::string						   uri_;
	std::string						   version_;
	std::map<std::string, std::string> headers_;
	std::string						   body_;
};
