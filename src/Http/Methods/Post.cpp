#include "Http/RequestHandler.hpp"
#include "Http/Helper.hpp"
#include "Http/HttpResponse.hpp"
#include "Http/HttpRequest.hpp"


#include <fstream>
#include <sstream>
#include <ctime>
#include <iostream>
#include "Http/Colors.hpp"




void RequestHandler::handlePost(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res)
{
    const std::string &contentType = req.header("content-type");

    std::cout << BOLDGREEN << "ContentType ---> [ " << contentType << " ]" << RESET << std::endl;

    if(contentType.find("multipart/form-data") != std::string::npos)
    {
        handleMultipart(req, loc, res);
        return;
    }

    std::cout << BOLDRED << "Unsupported Content-Type" << RESET << std::endl;
    handleError(415, res); // Unsupported Media Type
}


void RequestHandler::handleMultipart(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res)
{
    const std::string &contentType = req.header("content-type");
    const std::string &body        = req.body();

    std::cout << BLUE << "handleMultipart" << RESET << std::endl;

    if(loc.upload_store.empty())
    {
        std::cout << GREEN << "empty upload_store" << RESET << std::endl;
        handleError(400, res);
        return;
    }

    const std::string boundary = RequestHelpers::extractBoundary(contentType);
    std::cout << YELLOW << "boundary [ " << boundary << " ]" << RESET << std::endl;
    if(boundary.empty())
    {
        std::cout << GREEN << "empty boundary" << RESET << std::endl;
        handleError(400, res);
        return;
    }

    const std::string dlm = "--" + boundary;
    int filesUploaded     = 0;
    std::size_t pos       = 0;

    while(true)
    {
        // this order from rfc 2046
        const std::size_t bdPos = body.find(dlm, pos);
        if(bdPos == std::string::npos)
            break;
        pos = bdPos + dlm.size();

        if(body.compare(pos, 2, "--") == 0)
            break;

        if(body.compare(pos, 2, "\r\n") == 0)
            pos += 2;

        const std::size_t endHeader = body.find("\r\n\r\n", pos);
        if(endHeader == std::string::npos)
            break;

        const std::string partHeader = body.substr(pos, endHeader - pos);
        pos = endHeader + 4;

        const std::size_t endBody = body.find("\r\n" + dlm, pos);
        if(endBody == std::string::npos)
            break;

        const std::string partBody = body.substr(pos, endBody - pos);
        pos = endBody + 2;

        std::string filename = RequestHelpers::extractFilename(RequestHelpers::getDisposition(partHeader));
        if(filename.empty())
            continue;

        const std::size_t slash = filename.rfind('/');
        if(slash != std::string::npos)
            filename = filename.substr(slash + 1);
        const std::size_t back = filename.rfind('\\');
        if(back != std::string::npos)
            filename = filename.substr(back + 1);
        if(filename.empty())
            filename = "upload_" + RequestHelpers::sizeToString(static_cast<std::size_t>(time(NULL)));

        const std::string dest = loc.upload_store + "/" + filename;

        std::ofstream out(dest.c_str(), std::ios::binary);
        if(!out.is_open())
        {
            std::cout << BOLDRED << "cannot open " << dest << RESET << std::endl;
            handleError(500, res);
            return;
        }

        out.write(partBody.c_str(), static_cast<std::streamsize>(partBody.size()));
        out.close();
        ++filesUploaded;
    }

    if(filesUploaded == 0)
    {
        std::cout << GREEN << "no files uploaded" << RESET << std::endl;
        handleError(400, res);
        return;
    }

    const std::string ok = "Files uploaded successfully.";
    res.setStatus(201);
    res.setHeader("Content-Type", "text/plain");
    res.setHeader("Content-Length", RequestHelpers::sizeToString(ok.size()));
    res.setBody(ok);
}


