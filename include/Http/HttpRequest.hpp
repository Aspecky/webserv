#pragma once

#include <cstddef>
#include <map>
#include <string>

class HttpRequest {
  public:
	enum State {
		PARSING_REQUEST_LINE,
		PARSING_HEADERS,
		PARSING_BODY,
		COMPLETE,
		ERROR
	};

	HttpRequest();
	~HttpRequest();

	bool feed(const char *data, size_t n);

	State state() const;
	bool  isComplete() const;
	bool  hasError() const;

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

	bool parseRequestLine();
	bool parseHeaders();
	bool parseBody();

	static std::string toLower(const std::string &s);
	static std::string trim(const std::string &s);

	State							   state_;
	std::string						   raw_;
	std::string						   method_;
	std::string						   uri_;
	std::string						   version_;
	std::map<std::string, std::string> headers_;
	std::string						   body_;
	size_t							   bodyLength_;
};
