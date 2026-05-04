#include "Response/RequestHandler.hpp"
#include "Response/Helper.hpp"
#include "Response/Response.hpp"
#include "Http/HttpRequest.hpp"

#include <iostream>




void RequestHandler::handleGet(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res, std::string &matched, bool withBody)
{
    // std::string relativePath = req.getPath().substr(loc.path.size());
    std::string relativePath = req.uri().substr(matched.size());
    
    std::string fullPath = loc.root;
    if (!fullPath.empty() && fullPath[fullPath.size() - 1] != '/') {
        if (relativePath.empty() || relativePath[0] != '/') {
            fullPath += '/';
        }
    }
    fullPath += relativePath;


    std::cout << "[ " << fullPath << " ]" << std::endl;

  // Is Directory
    if(isDirectory(fullPath))
    {

        std::string indexPath = fullPath;
        if(fullPath.empty() || indexPath[indexPath.size() -1] != '/')
          indexPath += '/';
        indexPath += loc.index.empty() ? "index.html" : loc.index;

        std::cout << "Full path --------> "  << indexPath << std::endl;
        if(fileExists(indexPath))
        {
            bool ok = false;
            std::string body = readFile(indexPath, ok);
            if(!ok){
                std::cout << "file do not opend\n";
                handleError(500, res);
                return;
            }
            res.setStatus(200);
            res.setHeader("content-type", mimeTypes(indexPath));
            res.setHeader("content-length", RequestHelpers::sizeToString(body.size()));
            if(withBody)
                res.setBody(body);
            return;
        }
        if(loc.directory_listing)
        {

            std::cout << "[ Directory listing ]\n";
            std::string body = buildDirectoryListing(fullPath, req.uri());
            res.setStatus(200);
            res.setHeader("content-type", "text/html");
            res.setHeader("content-length", RequestHelpers::sizeToString(body.size()));
            if(withBody)
                res.setBody(body);
          return;
        }

        handleError(403, res);
          return;

    }
    if(!fileExists(fullPath))
    {
      handleError(404, res);
      return;
    }

    
    bool    ok = false;
    std::cout << "full path if is not directory ----------> " << fullPath << std::endl;
    std::string body                         = readFile(fullPath, ok);

    res.setStatus(200);
    res.setHeader("content-type", mimeTypes(fullPath));
    res.setHeader("content-length", RequestHelpers::sizeToString(body.size()));
    if(withBody)
        res.setBody(body);
}