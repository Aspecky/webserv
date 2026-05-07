#pragma once

#include "Http/HttpRequest.hpp"
#include "Http/Reader.hpp"
#include <cstddef>
#include <string>

class HttpParser {
  public:
	HttpParser();
	~HttpParser();

	bool feed(const char *data, size_t n, HttpRequest &req);
	void reset();
	bool isComplete() const;
	bool hasError() const;

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

	Reader		r_;
	State		state_;
	std::string buf_;
	size_t		bodyLength_;
};
