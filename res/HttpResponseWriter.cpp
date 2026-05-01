#include "HttpResponseWriter.hpp"

#include "HttpResponse.hpp"

#include <cstddef>
#include <map>
#include <sstream>
#include <string>

namespace {

std::string sizeToString(std::size_t n)
{
	std::ostringstream ss;
	ss << n;
	return ss.str();
}

} // namespace

void HttpResponseWriter::serialize(const HttpResponse &res,
								   std::string		  &out,
								   bool				   closeConnection)
{
	std::ostringstream wire;

	wire << "HTTP/1.1 " << res.statusCode() << ' ' << res.statusReason() << "\r\n";

	// Headers map is keyed lowercase; emit names as-stored.
	const std::map<std::string, std::string>		   &headers = res.headers();
	std::map<std::string, std::string>::const_iterator it;

	// Auto-fill Content-Length if the handler did not set it and a body exists.
	const bool needsContentLength =
		!res.body().empty() && headers.find("content-length") == headers.end();

	for (it = headers.begin(); it != headers.end(); ++it)
		wire << it->first << ": " << it->second << "\r\n";

	if (needsContentLength)
		wire << "content-length: " << sizeToString(res.body().size()) << "\r\n";

	if (headers.find("connection") == headers.end())
		wire << "connection: " << (closeConnection ? "close" : "keep-alive") << "\r\n";

	wire << "\r\n";
	wire << res.body();

	out.append(wire.str());
}
