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

// request-line = method SP request-target SP HTTP-version
// request-line CRLF
HttpParser::ParseResult HttpParser::tryParseRequestLine(HttpRequest &req)
{
	if (!r_.contains("\r\n")) {
		return INCOMPLETE;
	}

	std::string out;

	if (!r_.captureRule(grammar::http::Method(), out)) {
		return PARSE_ERROR;
	}
	req.method(out);

	if (!r_.consume(grammar::abnf::SP)) {
		return PARSE_ERROR;
	}

	if (!r_.captureRule(grammar::http11::RequestTarget(), out)) {
		return PARSE_ERROR;
	}
	req.uri(out);

	if (!r_.consume(grammar::abnf::SP)) {
		return PARSE_ERROR;
	}

	if (!r_.captureRule(grammar::http11::HttpVersion(), out)) {
		return PARSE_ERROR;
	}
	req.version(out);

	if (!r_.consumeRule(grammar::abnf::CRLF())) {
		return PARSE_ERROR;
	}

	return COMPLETE;
}

// field-line = field-name ":" OWS field-value OWS
// *( field-line CRLF ) CRLF
HttpParser::ParseResult HttpParser::tryParseHeaders(HttpRequest &req)
{
	if (!r_.contains("\r\n\r\n")) {
		return INCOMPLETE;
	}

	while (r_.remaining() >= 2 && (r_.pos[0] != '\r' || r_.pos[1] != '\n')) {
		std::string name, value;
		if (!r_.captureRule(grammar::http::FieldName(), name)) {
			return PARSE_ERROR;
		}
		if (!r_.consume(':')) {
			return PARSE_ERROR;
		}
		if (!r_.consumeRule(grammar::http::OWS())) {
			return PARSE_ERROR;
		}
		if (!r_.captureRule(grammar::http::FieldValue(), value)) {
			return PARSE_ERROR;
		}
		if (!r_.consumeRule(grammar::http::OWS())) {
			return PARSE_ERROR;
		}
		if (!r_.consumeRule(grammar::abnf::CRLF())) {
			return PARSE_ERROR;
		}
		req.header(name, value);
	}

	if (!r_.consumeRule(grammar::abnf::CRLF())) {
		return PARSE_ERROR;
	}

	return COMPLETE;
}

// message-body = *OCTET
// [ message-body ]
HttpParser::ParseResult HttpParser::tryParseBody(HttpRequest &req)
{
	if (r_.remaining() < bodyLength_) {
		return INCOMPLETE;
	}
	req.body(std::string(r_.pos, bodyLength_));
	r_.pos += bodyLength_;
	return COMPLETE;
}

// HTTP-request = request-line CRLF *( field-line CRLF ) CRLF [ message-body ]
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
			if (req.hasHeader("content-length")) {
				bodyLength_ = static_cast<size_t>(
					std::strtoul(req.header("content-length").c_str(), NULL, 10));
			}
			state_ = (bodyLength_ == 0) ? PARSING_DONE : PARSING_BODY;
		}
		else if (state_ == PARSING_BODY) {
			state_ = PARSING_DONE;
		}
	}

	buf_.erase(0, r_.consumed());
	return state_ != PARSING_ERROR;
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
