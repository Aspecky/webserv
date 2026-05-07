#include "Response/RequestHandler.hpp"
#include "Response/Helper.hpp"
#include "Response/Response.hpp"
#include "Http/HttpRequest.hpp"

#include <cstdio>
#include <iostream>


void RequestHandler::handleDelete(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res, const std::string &matched)
{
    std::string relativePath = RequestHelpers::urlDecode(req.uri().substr(matched.size()));

    std::string fullPath = loc.root;
    if(!fullPath.empty() && fullPath[fullPath.size() - 1] != '/')
    {
        if(relativePath.empty() || relativePath[0] != '/')
            fullPath += '/';
    }
    fullPath += relativePath;

    std::cout << "[ DELETE " << fullPath << " ]" << std::endl;

    if(isDirectory(fullPath))
    {
        handleError(403, res);
        return;
    }

    if(!fileExists(fullPath))
    {
        handleError(404, res);
        return;
    }

    if(std::remove(fullPath.c_str()) != 0)
    {
        handleError(500, res);
        return;
    }

    const std::string body = "File deleted.";
    res.setStatus(200);
    res.setHeader("Content-Type", "text/plain");
    res.setHeader("Content-Length", RequestHelpers::sizeToString(body.size()));
    res.setBody(body);
}
