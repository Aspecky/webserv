#ifndef HTTPRESPONSEWRITER_HPP
#define HTTPRESPONSEWRITER_HPP


#include <string>

class HttpResponse;



struct HttpResponseWriter {
    static void serialize(const HttpResponse& res,
                            std::string& out,
                            bool closeConnection = false);
};

#endif
