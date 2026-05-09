#pragma once

#include "Http/HttpRequest.hpp"
#include "Http/Reader.hpp"
#include <cstddef>
#include <string>

class HttpParser {
  public:
	HttpParser(size_t maxBodySize);
	~HttpParser();

	bool feed(const char *data, size_t n, HttpRequest &req);
	void reset();
	bool isComplete() const;
	bool hasError() const;
	int	 statusCode() const;

	static const int SUPPORTED_HTTP_VERSION = 1;

  private:
	HttpParser(const HttpParser &other);
	HttpParser &operator=(HttpParser other);

	enum State {
		PARSING_REQUEST_LINE,
		PARSING_HEADERS,
		PARSING_BODY,
		PARSING_DONE,
		PARSING_ERROR
	};

	enum ParseResult {
		INCOMPLETE,
		COMPLETE,
		PARSE_ERROR
	};

	ParseResult tryParseRequestLine_(HttpRequest &req);
	ParseResult tryParseHeaders_(HttpRequest &req);
	ParseResult tryParseBody_(HttpRequest &req);

	static bool validateHttpVersion_(const std::string &str);

	const size_t MAX_BODY_SIZE;
	Reader		 r_;
	State		 state_;
	std::string	 buf_;
	size_t		 bodySize_;
	int			 statusCode_;
};
