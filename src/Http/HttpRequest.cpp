#include "Http/HttpRequest.hpp"
#include <map>
#include <string>

HttpRequest::HttpRequest()
{
}

HttpRequest::~HttpRequest()
{
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
	return headers_.count(name) > 0;
}

const std::string &HttpRequest::header(const std::string &name) const
{
	return headers_.find(name)->second;
}
