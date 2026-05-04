#include "Response/RequestHandler.hpp"
#include "Response/Helper.hpp"
#include "Response/Response.hpp"
#include "Http/HttpRequest.hpp"



#include <fstream>
#include <sstream>
#include <ctime>
#include <iostream>


#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"   

void RequestHandler::handlePost(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res)
{
    const std::string &contentType = req.header("content-type");
    const std::string &body        = req.body();

    std::cout << BOLDGREEN << "ContentType ---> [ " << contentType << " ]"<< RESET << std::endl;
    // std::cout << "Body : \n" << body << std::endl;



    if(contentType.find("multipart/form-data") != std::string::npos)
    {
        std::cout << BLUE << "Find multipart/form-data " << RESET << std::endl;
        if(loc.upload_store.empty())
        {
            std::cout << GREEN << "form upload_store" << RESET<< std::endl;
            handleError(400, res);
            return;
        }

        const std::string boundary = RequestHelpers::extractBoundary(contentType);
        std::cout << YELLOW <<  "[ " << boundary << " ]" << RESET << std::endl;
        if(boundary.empty())
        {
            std::cout << GREEN << "form extracBoundary" << RESET<< std::endl;
            handleError(400, res);
            return ;
        }
        
        const std::string dlm   = "--" + boundary;
        int filesUploaded       = 0;
        std::size_t pos         = 0;

        while(true){
            const std::size_t bdPos = body.find(dlm, pos);
            if(bdPos == std::string::npos)
                break;
            pos = bdPos + dlm.size();

            if(body.compare(pos,2, "--") == 0)
                break;
            
            if(body.compare(pos, 2, "\r\n") == 0)
                pos += 2;
            
            const std::size_t endHeader = body.find("\r\n\r\n", pos);
            if(endHeader == std::string::npos)
                break;

            const std::string partHeader = body.substr(pos, endHeader - pos);
            pos = endHeader + 4;

            const std::size_t endBody = body.find("\r\n" +  dlm, pos);
            if(endBody == std::string::npos)
                break;

            const std::string  partBody = body.substr(pos, endBody - pos);
            pos = endBody + 2;

            std::string filename = RequestHelpers::extractFilename(RequestHelpers::getDisposition(partHeader));
            if(filename.empty())
                continue;
            
            const std::size_t slash = filename.rfind('/');
            if(slash != std::string::npos)
                filename = filename.substr(slash + 1);
            if(filename.empty())
                filename = "upload_" + RequestHelpers::sizeToString(static_cast<std::size_t>(time(NULL)));

            const std::string dest = loc.upload_store + "/" + filename;
            
            std::ofstream out(dest.c_str(), std::ios::binary);
            if(!out.is_open())
            {
                
                handleError(500, res);
                return;
            }

            out.write(partBody.c_str(), static_cast<std::streamsize>(partBody.size()));
            out.close();
            ++filesUploaded;
        }

        // if(filesUploaded == 0)
        // {
        //     std::cout << GREEN << "files uploaded " << RESET << std::endl;
        //     handleError(400, res);
        //     return;
        // }

        const std::string ok = "Files uploaded successfully.";
        res.setStatus(201);
        res.setHeader("Content-Type", "text/plain");
		res.setHeader("Content-Length", RequestHelpers::sizeToString(ok.size()));
		res.setBody(ok);
        return;

    } // multipart/form-data

    if(contentType.find("application/x-www-form-urlencoded") != std::string::npos)
    {
        if(body.empty())
        {
            std::cout << BLUE << "empty body " << RESET << std::endl;
            handleError(400, res);
            return;
        }

        std::ostringstream html;
        html << "<html><body><h2>Form Received</h2><ul>";
        std::string remaining = body;
        while(!remaining.empty())
        {
            const std::size_t amp = remaining.find('&');
            const std::string pair   = (amp == std::string::npos) ? remaining : remaining.substr(0, amp);
            remaining = (amp == std::string::npos) ? "" : remaining.substr(amp + 1);

            const std::size_t eq = pair.find('=');
            if(eq != std::string::npos)
            {
                html << "<li><b>" << pair.substr(0, eq) << "</b>: "
					 << pair.substr(eq + 1) << "</li>";
			}

        }

        html << "</ul></body></html>";
        const std::string out = html.str();

        res.setStatus(200);
		res.setHeader("Content-Type", "text/html");
		res.setHeader("Content-Length", RequestHelpers::sizeToString(out.size()));
		res.setBody(out);
		return;
    }
    if(loc.upload_store.empty())
    {   
        std::cout << BLUE << "empty loc.upload_store " << RESET << std::endl;
        handleError(400, res);
        return;
    }
    const std::string dest = loc.upload_store + "/upload_" + RequestHelpers::sizeToString(static_cast<std::size_t>(time(NULL)));
    std::ofstream out(dest.c_str(), std::ios::binary);

    if(!out.is_open())
    {
        std::cout << BOLDRED<< dest << RESET << std::endl;
        handleError(500, res);
        return ;
    }
    out.write(body.c_str(), static_cast<std::streamsize>(body.size()));
    out.close();

    const std::string ok = "Uploaded successfully.";
	res.setStatus(201);
	res.setHeader("Content-Type", "text/plain");
	res.setHeader("Content-Length", RequestHelpers::sizeToString(ok.size()));
	res.setBody(ok);
}