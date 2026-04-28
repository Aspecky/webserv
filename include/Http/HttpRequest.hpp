#pragma once

#include <map>
#include <string>

class HttpRequest {
  public:
	HttpRequest();
	~HttpRequest();

	const std::string &method() const;
	void               method(const std::string &method);

	const std::string &uri() const;
	void               uri(const std::string &uri);

	const std::string &version() const;
	void               version(const std::string &version);

	const std::string &body() const;
	void               body(const std::string &body);

	bool			   hasHeader(const std::string &name) const;
	const std::string &header(const std::string &name) const;
	void               header(const std::string &name, const std::string &value);
	const std::map<std::string, std::string> &headers() const;

  private:
	HttpRequest(const HttpRequest &);
	HttpRequest &operator=(const HttpRequest &);

	std::string						   method_;
	std::string						   uri_;
	std::string						   version_;
	std::map<std::string, std::string> headers_;
	std::string						   body_;
};
