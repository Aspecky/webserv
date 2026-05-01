#include "HttpResponse.hpp"

#include <algorithm>
#include <cctype>

namespace {

struct ReasonRow {
	int			code;
	const char *phrase;
};

const ReasonRow kReasonTable[] = {
	{200, "OK"},
	{201, "Created"},
	{204, "No Content"},
	{301, "Moved Permanently"},
	{302, "Found"},
	{303, "See Other"},
	{304, "Not Modified"},
	{307, "Temporary Redirect"},
	{308, "Permanent Redirect"},
	{400, "Bad Request"},
	{401, "Unauthorized"},
	{403, "Forbidden"},
	{404, "Not Found"},
	{405, "Method Not Allowed"},
	{408, "Request Timeout"},
	{411, "Length Required"},
	{413, "Payload Too Large"},
	{414, "URI Too Long"},
	{415, "Unsupported Media Type"},
	{500, "Internal Server Error"},
	{501, "Not Implemented"},
	{502, "Bad Gateway"},
	{503, "Service Unavailable"},
	{504, "Gateway Timeout"},
	{505, "HTTP Version Not Supported"},
};

const std::string kEmpty;

std::string defaultReason(int code)
{
	const std::size_t n = sizeof(kReasonTable) / sizeof(kReasonTable[0]);
	for (std::size_t i = 0; i < n; ++i) {
		if (kReasonTable[i].code == code)
			return kReasonTable[i].phrase;
	}
	return "";
}

std::string toLower(const std::string &s)
{
	std::string out(s);
	for (std::size_t i = 0; i < out.size(); ++i)
		out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
	return out;
}

} // namespace

HttpResponse::HttpResponse()
	: statusCode_(200), statusReason_("OK")
{
}

void HttpResponse::setStatus(int code)
{
	statusCode_	  = code;
	statusReason_ = defaultReason(code);
}

void HttpResponse::setStatus(int code, const std::string &reason)
{
	statusCode_	  = code;
	statusReason_ = reason;
}

void HttpResponse::setHeader(const std::string &name, const std::string &value)
{
	headers_[toLower(name)] = value;
}

void HttpResponse::setBody(const std::string &body)
{
	body_ = body;
}

void HttpResponse::appendBody(const char *data, std::size_t n)
{
	body_.append(data, n);
}

int HttpResponse::statusCode() const
{
	return statusCode_;
}

const std::string &HttpResponse::statusReason() const
{
	return statusReason_;
}

const std::map<std::string, std::string> &HttpResponse::headers() const
{
	return headers_;
}

bool HttpResponse::hasHeader(const std::string &name) const
{
	return headers_.find(toLower(name)) != headers_.end();
}

const std::string &HttpResponse::header(const std::string &name) const
{
	std::map<std::string, std::string>::const_iterator it =
		headers_.find(toLower(name));
	if (it == headers_.end())
		return kEmpty;
	return it->second;
}

const std::string &HttpResponse::body() const
{
	return body_;
}

void HttpResponse::clear()
{
	statusCode_	  = 200;
	statusReason_ = "OK";
	headers_.clear();
	body_.clear();
}
