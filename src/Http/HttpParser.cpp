#include "Http/HttpParser.hpp"
#include "Http/Grammar.hpp"
#include "Http/HttpRequest.hpp"
#include "Http/Reader.hpp"
#include "Http/StatusCodes.hpp"
#include <cstddef>
#include <cstdlib>
#include <string>

HttpParser::HttpParser(size_t maxBodySize)
	: MAX_BODY_SIZE(maxBodySize), r_(0, 0), state_(PARSING_REQUEST_LINE),
	  bodySize_(0), statusCode_(status_codes::BAD_REQUEST)
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
	bodySize_	= 0;
	statusCode_ = status_codes::BAD_REQUEST;
}

// HTTP-version   = HTTP-name "/" DIGIT "." DIGIT
bool HttpParser::validateHttpVersion_(const std::string &str)
{
	Reader r(str.data(), str.size());

	r.consumeLiteral("HTTP");
	r.consume('/');

	return r.peek() - '0' == HttpParser::SUPPORTED_HTTP_VERSION;
}

// request-line = method SP request-target SP HTTP-version
// request-line CRLF
HttpParser::ParseResult HttpParser::tryParseRequestLine_(HttpRequest &req)
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
	{
		std::string::size_type qpos = out.find('?');
		if (qpos != std::string::npos) {
			req.path(out.substr(0, qpos));
			req.query(out.substr(qpos + 1));
		} else {
			req.path(out);
			req.query("");
		}
	}

	if (!r_.consume(grammar::abnf::SP)) {
		return PARSE_ERROR;
	}

	if (!r_.captureRule(grammar::http11::HttpVersion(), out)) {
		return PARSE_ERROR;
	}
	req.version(out);

	if (!validateHttpVersion_(req.version())) {
		statusCode_ = status_codes::HTTP_VERSION_NOT_SUPPORTED;
		return PARSE_ERROR;
	}

	if (!r_.consumeRule(grammar::abnf::CRLF())) {
		return PARSE_ERROR;
	}

	return COMPLETE;
}

// field-line = field-name ":" OWS field-value OWS
// *( field-line CRLF ) CRLF
HttpParser::ParseResult HttpParser::tryParseHeaders_(HttpRequest &req)
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
HttpParser::ParseResult HttpParser::tryParseBody_(HttpRequest &req)
{
	if (r_.remaining() < bodySize_) {
		return INCOMPLETE;
	}
	req.body(std::string(r_.pos, bodySize_));
	r_.pos += bodySize_;
	return COMPLETE;
}

// HTTP-request = request-line CRLF *( field-line CRLF ) CRLF [ message-body ]
bool HttpParser::feed(const char *data, size_t n, HttpRequest &req)
{
	if (state_ == PARSING_DONE || state_ == PARSING_ERROR) {
		return state_ != PARSING_ERROR;
	}

	buf_.append(data, n);
	r_ = Reader(buf_.data(), buf_.size());

	while (state_ != PARSING_DONE && state_ != PARSING_ERROR) {
		ParseResult result = INCOMPLETE;
		if (state_ == PARSING_REQUEST_LINE) {
			result = tryParseRequestLine_(req);
		}
		else if (state_ == PARSING_HEADERS) {
			result = tryParseHeaders_(req);
		}
		else if (state_ == PARSING_BODY) {
			result = tryParseBody_(req);
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
				bodySize_ = static_cast<size_t>(std::strtoul(
					req.header("content-length").c_str(), NULL, 10));
				if (bodySize_ > MAX_BODY_SIZE) {
					statusCode_ = status_codes::CONTENT_TOO_LARGE;
					return PARSING_ERROR;
				}
			}
			state_ = (bodySize_ == 0) ? PARSING_DONE : PARSING_BODY;
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

int HttpParser::statusCode() const
{
	return statusCode_;
}
