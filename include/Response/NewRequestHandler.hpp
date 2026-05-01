#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP


#include "Config/ConfigTypes.hpp"
#include <string>

class HttpRequest;
class HttpResponse;

class RequestHandler {
    public:
        explicit RequestHandler(const ServerConfig &confg);

        void handle(const HttpRequest &req, HttpResponse &res);

        void handleError(int status, HttpResponse &res);


    private:
        RequestHandler(const RequestHandler &);
        RequestHandler& operator=(const RequestHandler& );


        const ServerConfig &config_;

        // Routing 
        const LocationConfig *matchLocation(const std::string &uri, std::string &matchPrefix) const;
        bool  isMethodAllowed(const LocationConfig &loc, const std::string &method) const;

        // Method dispatch
        void handleGet(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res, bool withBody);
        void handleHead(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res);
        void handlePost(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res);
        void handleDelete(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res);

        // Filesystem helpers
        std::string buildDirectoryListing(const std::string &path, const std::string &url) const;
        std::string mimeTypes(const std::string &path)          const;
        std::string readFile(const std::string &path, bool &ok) const;
        bool        isDirectory(const std::string &path)        const;
        bool        fileExists(const std::string &path)         const;

        void writeErrorBody(int status, HttpResponse &res) const;

};

#endif
