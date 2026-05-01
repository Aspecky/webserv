

#include "Response/NewResponse.hpp"
#include "Response/NewHttpResponseWriter.hpp"

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
} // end namespace

void HttpResponseWriter::serialize(const HttpResponse &res,
                                    std::string &out,
                                    bool closeConnection)
{
    std::ostringstream wire;

    wire << "HTTP/1.1 " << res.statusCode() << ' ' << res.statusReason() << "\r\n";

    const std::map<std::string, std::string>        headers= res.headers();

    std::map<std::string, std::string>::const_iterator it;

    const bool needContentLength = 
        !res.body().empty() && headers.find("content-length") == headers.end();

    for(it = headers.begin(); it != headers.end(); ++it)
        wire << it->first << ": " << it->second << "\r\n";

    if(needContentLength)
        wire << "contenr-length:" << sizeToString(res.body().size()) << "\r\n";

    if(headers.find("connection") == headers.end())
        wire << "connection: " << (closeConnection ? "close" : "keep-alive") << "\r\n";

    wire << "\r\n";
    wire << res.body();
    
    out.append(wire.str());
}

