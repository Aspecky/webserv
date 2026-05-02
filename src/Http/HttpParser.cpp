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

bool HttpParser::feed(HttpRequest &req, const char *data, size_t n)
{
	if (state_ == PARSING_DONE || state_ == PARSING_ERROR) {
		return state_ != PARSING_ERROR;
	}

	buf_.append(data, n);
	r_ = Reader(buf_.data(), buf_.size());

	bool progress = true;
	while (progress && state_ != PARSING_DONE && state_ != PARSING_ERROR) {
		progress = false;

		if (state_ == PARSING_REQUEST_LINE) {
			if (!r_.contains("\r\n")) {
				break;
			}
			if (!parseRequestLine(req)) {
				state_ = PARSING_ERROR;
				break;
			}
			state_	 = PARSING_HEADERS;
			progress = true;
		}
		else if (state_ == PARSING_HEADERS) {
			if (!r_.contains("\r\n\r\n")) {
				break;
			}
			if (!parseHeaders(req)) {
				state_ = PARSING_ERROR;
				break;
			}
			state_	 = PARSING_BODY;
			progress = true;
		}
		else if (state_ == PARSING_BODY) {
			if (req.hasHeader("content-length")) {
				bodyLength_ = static_cast<size_t>(std::strtoul(
					req.header("content-length").c_str(), NULL, 10));
			}
			if (r_.remaining() < bodyLength_) {
				break;
			}
			parseBody(req);
			state_	 = PARSING_DONE;
			progress = true;
		}
	}

	buf_.erase(0, r_.consumed());
	return state_ != PARSING_ERROR;
}

// request-line   = method SP request-target SP HTTP-version
bool HttpParser::parseRequestLine(HttpRequest &req)
{
	std::string out;

	if (!parseToken(out)) {
		return false;
	}
	req.method(out);
	out.clear();

	if (!r_.consume(grammar::abnf::SP)) {
		return false;
	}

	if (!parseRequestTarget(out)) {
		return false;
	}
	req.uri(out);
	out.clear();

	if (!r_.consume(grammar::abnf::SP)) {
		return false;
	}

	if (!parseHttpVersion(out)) {
		return false;
	}
	req.version(out);
	out.clear();

	return parseCRLF();
}

bool HttpParser::parseHeaders(HttpRequest &req)
{
	while (r_.remaining() >= 2 && (r_.pos[0] != '\r' || r_.pos[1] != '\n')) {
		std::string name, value;
		if (!parseFieldLine(name, value)) {
			return false;
		}
		req.header(name, value);
	}
	return parseCRLF();
}

// message-body = *OCTET (length determined by Content-Length header)
void HttpParser::parseBody(HttpRequest &req)
{
	req.body(std::string(r_.pos, bodyLength_));
	r_.pos += bodyLength_;
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
