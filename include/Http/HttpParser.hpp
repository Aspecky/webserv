#pragma once

#include "Http/HttpRequest.hpp"
#include "Http/Reader.hpp"
#include <cstddef>
#include <string>

class HttpParser {
  public:
	HttpParser();
	~HttpParser();

	bool feed(HttpRequest &req, const char *data, size_t n);
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

	bool parseCRLF();
	bool parseToken(std::string &out);
	bool parseRequestTarget(std::string &out);
	bool parseHttpVersion(std::string &out);
	bool parseFieldLine(std::string &name, std::string &value);

	ParseResult tryParseRequestLine(HttpRequest &req);
	ParseResult tryParseHeaders(HttpRequest &req);
	ParseResult tryParseBody(HttpRequest &req);

	Reader		r_;
	State		state_;
	std::string buf_;
	size_t		bodyLength_;
};
