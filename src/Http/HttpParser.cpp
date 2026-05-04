#include "Http/HttpParser.hpp"
#include "Http/Grammar.hpp"
#include "Http/HttpRequest.hpp"
#include "Http/Reader.hpp"
#include <cstddef>
#include <cstdlib>
#include <string>

HttpParser::HttpParser()
	: r_(0, 0), state_(PARSING_REQUEST_LINE), bodyLength_(0)
{
}

HttpParser::~HttpParser()
{
}

void HttpParser::reset()
{
	buf_.clear();
	r_			= Reader(0, 0);
	state_		= PARSING_REQUEST_LINE;
	bodyLength_ = 0;
}

bool HttpParser::parseCRLF()
{
	return r_.consume('\r') && r_.consume('\n');
}

HttpParser::ParseResult HttpParser::tryParseRequestLine(HttpRequest &req)
{
	if (!r_.contains("\r\n")) {
		return INCOMPLETE;
	}

	std::string out;

	if (!parseToken(out)) {
		return PARSE_ERROR;
	}
	req.method(out);

	if (!r_.consume(grammar::abnf::SP)) {
		return PARSE_ERROR;
	}

	out.clear();
	if (!parseRequestTarget(out)) {
		return PARSE_ERROR;
	}
	req.uri(out);

	if (!r_.consume(grammar::abnf::SP)) {
		return PARSE_ERROR;
	}

	out.clear();
	if (!parseHttpVersion(out)) {
		return PARSE_ERROR;
	}
	req.version(out);

	if (!parseCRLF()) {
		return PARSE_ERROR;
	}
	return COMPLETE;
}

HttpParser::ParseResult HttpParser::tryParseHeaders(HttpRequest &req)
{
	if (!r_.contains("\r\n\r\n")) {
		return INCOMPLETE;
	}

	while (r_.remaining() >= 2 && (r_.pos[0] != '\r' || r_.pos[1] != '\n')) {
		std::string name, value;
		if (!parseFieldLine(name, value)) {
			return PARSE_ERROR;
		}
		req.header(name, value);
	}
	if (!parseCRLF()) {
		return PARSE_ERROR;
	}
	return COMPLETE;
}

HttpParser::ParseResult HttpParser::tryParseBody(HttpRequest &req)
{
	if (req.hasHeader("content-length")) {
		bodyLength_ = static_cast<size_t>(
			std::strtoul(req.header("content-length").c_str(), NULL, 10));
	}
	if (r_.remaining() < bodyLength_) {
		return INCOMPLETE;
	}
	req.body(std::string(r_.pos, bodyLength_));
	r_.pos += bodyLength_;
	return COMPLETE;
}

bool HttpParser::feed(HttpRequest &req, const char *data, size_t n)
{
	if (state_ == PARSING_DONE || state_ == PARSING_ERROR) {
		return state_ != PARSING_ERROR;
	}

	buf_.append(data, n);
	r_ = Reader(buf_.data(), buf_.size());

	while (state_ != PARSING_DONE && state_ != PARSING_ERROR) {
		ParseResult result = INCOMPLETE;
		if (state_ == PARSING_REQUEST_LINE) {
			result = tryParseRequestLine(req);
		}
		else if (state_ == PARSING_HEADERS) {
			result = tryParseHeaders(req);
		}
		else if (state_ == PARSING_BODY) {
			result = tryParseBody(req);
		}

		if (result == PARSE_ERROR) {
			state_ = PARSING_ERROR;
			break;
		}
		if (result == INCOMPLETE) {
			break;
		}

		if (state_ == PARSING_REQUEST_LINE) {
			state_ = PARSING_HEADERS;
		}
		else if (state_ == PARSING_HEADERS) {
			state_ = PARSING_BODY;
		}
		else if (state_ == PARSING_BODY) {
			state_ = PARSING_DONE;
		}
	}

	buf_.erase(0, r_.consumed());
	return state_ != PARSING_ERROR;
}

/*
	field-line = field-name ":" OWS field-value OWS
	field-name = token
*/
bool HttpParser::parseFieldLine(std::string &name, std::string &value)
{
	if (!parseToken(name)) {
		return false;
	}

	if (!r_.consume(':')) {
		return false;
	}

	if (!r_.consumeRule(grammar::http::OWS())) {
		return false;
	}

	const char *start = r_.pos;
	if (!r_.consumeRule(grammar::http::FieldValue())) {
		return false;
	}
	value = std::string(start, static_cast<size_t>(r_.pos - start));

	if (!r_.consumeRule(grammar::http::OWS())) {
		return false;
	}

	return parseCRLF();
}

// token = 1*tchar
bool HttpParser::parseToken(std::string &out)
{
	const char *start = r_.pos;
	if (!r_.consumeWhileRule(grammar::http::Token())) {
		return false;
	}
	out = std::string(start, static_cast<size_t>(r_.pos - start));
	return true;
}

// TODO: Add the other 3 forms
/*
	request-target = origin-form
				   / absolute-form
				   / authority-form
				   / asterisk-form
*/
bool HttpParser::parseRequestTarget(std::string &out)
{
	const char *start = r_.pos;
	if (!r_.consumeRule(grammar::uri::OriginForm())) {
		return false;
	}
	out = std::string(start, static_cast<size_t>(r_.pos - start));
	return true;
}

bool HttpParser::parseHttpVersion(std::string &out)
{
	const char *start = r_.pos;
	if (!r_.consumeRule(grammar::http::HttpVersion())) {
		return false;
	}
	out = std::string(start, static_cast<size_t>(r_.pos - start));
	return true;
}

// MARK: Getters

bool HttpParser::isComplete() const
{
	return state_ == PARSING_DONE;
}

bool HttpParser::hasError() const
{
	return state_ == PARSING_ERROR;
}
