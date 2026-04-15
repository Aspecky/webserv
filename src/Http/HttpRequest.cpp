#include "Http/HttpRequest.hpp"
#include <cctype>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>

HttpRequest::HttpRequest() : state_(PARSING_REQUEST_LINE), bodyLength_(0)
{
}

HttpRequest::~HttpRequest()
{
}

bool HttpRequest::feed(const char *data, size_t n)
{
	if (state_ == COMPLETE || state_ == ERROR) {
		return state_ != ERROR;
	}

	raw_.append(data, n);

	bool progress = true;
	while (progress && state_ != COMPLETE && state_ != ERROR) {
		progress = false;
		if (state_ == PARSING_REQUEST_LINE) {
			progress = parseRequestLine();
		}
		else if (state_ == PARSING_HEADERS) {
			progress = parseHeaders();
		}
		else if (state_ == PARSING_BODY) {
			progress = parseBody();
		}
	}

	return state_ != ERROR;
}

bool HttpRequest::parseRequestLine()
{
	std::string::size_type end = raw_.find("\r\n");
	if (end == std::string::npos) {
		return false;
	}

	std::string line = raw_.substr(0, end);
	raw_.erase(0, end + 2);

	std::string::size_type sp1 = line.find(' ');
	if (sp1 == std::string::npos) {
		state_ = ERROR;
		return false;
	}

	std::string::size_type sp2 = line.find(' ', sp1 + 1);
	if (sp2 == std::string::npos) {
		state_ = ERROR;
		return false;
	}

	method_	 = line.substr(0, sp1);
	uri_	 = line.substr(sp1 + 1, sp2 - sp1 - 1);
	version_ = line.substr(sp2 + 1);

	if (method_.empty() || uri_.empty() || version_.empty()) {
		state_ = ERROR;
		return false;
	}

	state_ = PARSING_HEADERS;
	return true;
}

bool HttpRequest::parseHeaders()
{
	std::string::size_type end = raw_.find("\r\n\r\n");
	if (end == std::string::npos) {
		return false;
	}

	std::string headerBlock = raw_.substr(0, end);
	raw_.erase(0, end + 4);

	std::string::size_type pos = 0;
	while (pos < headerBlock.size()) {
		std::string::size_type lineEnd = headerBlock.find("\r\n", pos);
		if (lineEnd == std::string::npos) {
			lineEnd = headerBlock.size();
		}

		std::string line = headerBlock.substr(pos, lineEnd - pos);
		pos				 = lineEnd + 2;

		if (line.empty()) {
			continue;
		}

		std::string::size_type colon = line.find(':');
		if (colon == std::string::npos) {
			state_ = ERROR;
			return false;
		}

		//! remove the triming from the key, it's an error
		std::string key	  = toLower(trim(line.substr(0, colon)));
		std::string value = trim(line.substr(colon + 1));
		headers_[key]	  = value;
	}

	std::map<std::string, std::string>::iterator it =
		headers_.find("content-length");
	if (it != headers_.end()) {
		char *endPtr = NULL;
		long  len	 = std::strtol(it->second.c_str(), &endPtr, 10);
		if (endPtr == it->second.c_str() || len < 0) {
			state_ = ERROR;
			return false;
		}
		bodyLength_ = static_cast<size_t>(len);
		state_		= PARSING_BODY;
	}
	else {
		bodyLength_ = 0;
		state_		= COMPLETE;
	}

	return true;
}

bool HttpRequest::parseBody()
{
	if (raw_.size() < bodyLength_) {
		return false;
	}

	body_ = raw_.substr(0, bodyLength_);
	raw_.erase(0, bodyLength_);
	state_ = COMPLETE;
	return true;
}

// MARK: Getters
const std::map<std::string, std::string> &HttpRequest::headers() const
{
	return headers_;
}

HttpRequest::State HttpRequest::state() const
{
	return state_;
}
bool HttpRequest::isComplete() const
{
	return state_ == COMPLETE;
}
bool HttpRequest::hasError() const
{
	return state_ == ERROR;
}

const std::string &HttpRequest::method() const
{
	return method_;
}
const std::string &HttpRequest::uri() const
{
	return uri_;
}
const std::string &HttpRequest::version() const
{
	return version_;
}
const std::string &HttpRequest::body() const
{
	return body_;
}

bool HttpRequest::hasHeader(const std::string &name) const
{
	return headers_.find(toLower(name)) != headers_.end();
}

const std::string &HttpRequest::header(const std::string &name) const
{
	static const std::string						   empty;
	std::map<std::string, std::string>::const_iterator it =
		headers_.find(toLower(name));
	if (it == headers_.end()) {
		return empty;
	}
	return it->second;
}

std::string HttpRequest::toLower(const std::string &s)
{
	std::string result = s;
	for (size_t i = 0; i < result.size(); ++i) {
		result[i] = static_cast<char>(
			std::tolower(static_cast<unsigned char>(result[i])));
	}
	return result;
}

std::string HttpRequest::trim(const std::string &s)
{
	std::string::size_type start = s.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) {
		return "";
	}
	std::string::size_type end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}
