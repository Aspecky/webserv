#include "Http/HttpRequest.hpp"
#include <cctype>
#include <cstddef>
#include <map>
#include <string>

HttpRequest::HttpRequest()
{
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::reset()
{
	method_.clear();
	uri_.clear();
	version_.clear();
	headers_.clear();
	body_.clear();
}

static std::string toLower(const std::string &s)
{
	std::string lower(s);
	for (size_t i = 0; i < lower.size(); ++i) {
		lower[i] = static_cast<char>(
			std::tolower(static_cast<unsigned char>(lower[i])));
	}
	return lower;
}

// MARK: Getters
const std::map<std::string, std::string> &HttpRequest::headers() const
{
	return headers_;
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
	return headers_.count(toLower(name)) > 0;
}

const std::string &HttpRequest::header(const std::string &name) const
{
	return headers_.find(toLower(name))->second;
}

// MARK: Setters
void HttpRequest::method(const std::string &method)
{
	method_ = method;
}

void HttpRequest::uri(const std::string &uri)
{
	uri_ = uri;
}

void HttpRequest::version(const std::string &version)
{
	version_ = version;
}

void HttpRequest::body(const std::string &body)
{
	body_ = body;
}

void HttpRequest::header(const std::string &name, const std::string &value)
{
	headers_[toLower(name)] = value;
}
