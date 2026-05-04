#include "Response/RequestHandler.hpp"
#include "Response/Helper.hpp"
#include "Response/Response.hpp"
#include "Http/HttpRequest.hpp"


#include <cstdio> // ADD THIS for std::remove



void RequestHandler::handleDelete(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res)
{
    const std::string relativePath = req.uri();
    const std::string fullPath = loc.root + relativePath;

    if(!fileExists(fullPath))
    {
        handleError(404, res);
        return ;
    }

    if(std::remove(fullPath.c_str()) != 0)
    {
        handleError(500, res);
        return ;
    }

    const std::string body = "File deleted.";


}
