
#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP



#include <string>
#include <map>

class HttpResponse {
    public:
        HttpResponse();

        // Writer (used by the handler)
        void setStatus(int code);
        void setStatus(int code, const std::string& reason);
        void setHeader(const std::string& name, const std::string& value);
        void setBody(const std::string& body);
        void appendBody(const char* data, size_t n);

        // Reader (used by the writer and by tests)
        int statusCode() const;
        const std::string& statusReason() const;
        const std::map<std::string, std::string>& headers() const;
        bool hasHeader(const std::string& name) const;
        const std::string& header(const std::string& name) const;
        const std::string& body() const;

        
        void clear();

    private:
        HttpResponse(const HttpResponse&);
        HttpResponse& operator=(const HttpResponse&);

        int                                statusCode_;
        std::string                        statusReason_;
        std::map<std::string, std::string> headers_;
        std::string                        body_;

};

#endif