#include "Response/NewRequestHandler.hpp"

#include "Config/ConfigTypes.hpp"
#include "Config/ConfigParser.hpp"
#include "Response/NewResponse.hpp"
#include "Http/HttpRequest.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <ios>
#include <sstream>
#include <sys/stat.h>
#include <iostream>
#include <unistd.h>
// colore



namespace {

std::string Empty;
std::string sizeToString(std::size_t n)
{
    std::ostringstream ss;
    ss << n;
    return ss.str();
}
std::string toLower(const std::string &s)
{
    std::string out(s);
    for(std::size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

std::string extractBoundary(const std::string &contentType)
{
    const std::size_t pos = contentType.find("boundary=");
    if(pos == std::string::npos)
        return Empty;

    std::string boundary = contentType.substr(pos + 9);
    if(!boundary.empty() && boundary[0] == '"')
        boundary = boundary.substr(1, boundary.size() - 2);

    return boundary;
}

std::string extractFilename(const std::string &disposition)
{
    const std::size_t pos = disposition.find("filename=\"");
    if(pos == std::string::npos)
        return Empty;
    const std::size_t start = pos + 10;
    const std::size_t end = disposition.find('"', start);
    if(end == std::string::npos)
        return Empty;
    return disposition.substr(start, end - start);
}

std::string getDisposition(const std::string &partHeaders)
{
    std::string lower = toLower(partHeaders);
    std::size_t pos = lower.find("content-didposition:");

    if(pos == std::string::npos)
        return Empty;

    const std::size_t start = pos+ + 20;
    const std::size_t end = lower.find("\r\n", start);
    std::string value = partHeaders.substr(start,
                        (end == std::string::npos ? partHeaders.size() : end) - start );

    const std::size_t s = value.find_first_not_of("\t");
    return (s == std::string::npos) ? Empty : value.substr(s);
}

struct MimeRow {
    const char *ext;
    const char *type;
};

const MimeRow MimeTable[] = {
    {"html", "text/html"},		   {"htm", "text/html"},
	{"css", "text/css"},		   {"txt", "text/plain"},
	{"xml", "text/xml"},		   {"js", "application/javascript"},
	{"json", "application/json"},  {"pdf", "application/pdf"},
	{"zip", "application/zip"},	   {"png", "image/png"},
	{"gif", "image/gif"},		   {"jpg", "image/jpeg"},
	{"jpeg", "image/jpeg"},		   {"webp", "image/webp"},
	{"svg", "image/svg+xml"},	   {"ico", "image/x-icon"},
	{"mp3", "audio/mpeg"},		   {"mp4", "video/mp4"},
	{"webm", "video/webm"},		   {"woff", "font/woff"},
	{"woff2", "font/woff2"},
};

} // namespace


RequestHandler::RequestHandler(const ServerConfig &config): config_(config) {}


// void printServerConfig(const ServerConfig& server);

void RequestHandler::handle(const HttpRequest &req, HttpResponse &res)
{
    res.clear();

    // printServerConfig(config_);
    // pause();

    std::string matched;
    std::cout << "URI -----> " << req.uri()<< std::endl;

    const LocationConfig *loc = matchLocation(req.uri(), matched);
    if(!loc){
        handleError(404, res);
        return;
    }

    std::cout << "Matched path is ---->" << matched << std::endl;
    

    if(!isMethodAllowed(*loc, req.method())){
        handleError(405, res);
        return;
    }

    if(!loc->redirect.empty())
    {
        res.setStatus(301);
        res.setHeader("Location", loc->redirect);
        return;
    }

    std::cout << "method -----> " << req.method() << std::endl;
    if(req.method() == "GET")
        handleGet(req, *loc, res,matched, true);
    else if(req.method() == "HEAD")
        handleHead(req, *loc, res);
    else if(req.method() == "POST")
        handlePost(req, *loc, res);
    else if(req.method() == "DELETE")
        handleDelete(req, *loc, res);
    else
    {
        std::cout << " ---> Here <----\n";
        handleError(501, res);
    }
}

const LocationConfig * RequestHandler::matchLocation(const std::string &uri, std::string &matchedPrefix) const
{
    const LocationConfig *loc = NULL;
    matchedPrefix.clear();

    const std::map<std::string, LocationConfig> &locations = config_.locations;
    for(std::map<std::string, LocationConfig>::const_iterator it = locations.begin();
            it != locations.end(); ++it)
    {
        const std::string &prefix = it-> first;
        if(uri.compare(0, prefix.size(), prefix) == 0 && prefix.size() > matchedPrefix.size()){
            matchedPrefix = prefix;
            loc = &it->second;
        }
    }
    return loc;
}

bool  RequestHandler::isMethodAllowed(const LocationConfig &loc, const std::string &method) const
{
    std::vector<std::string>  vec = loc.methods;
    for(std::size_t i = 0; i < vec.size(); ++i)
    {
        if(vec[i] == method)
            return true;
    }
    return false;
    
}

void RequestHandler::handlePost(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res)
{
    const std::string &contentType = req.header("content-type");
    const std::string &body        = req.body();

    if(contentType.find("multipart/form-data") != std::string::npos)
    {
        if(loc.upload_store.empty())
        {
            handleError(400, res);
            return;
        }

        const std::string boundary = extractBoundary(contentType);
        if(boundary.empty())
        {
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

            std::string filename = extractFilename(getDisposition(partHeader));
            if(filename.empty())
                continue;
            
            const std::size_t slash = filename.rfind('/');
            if(slash != std::string::npos)
                filename = filename.substr(slash + 1);
            if(filename.empty())
                filename = "upload_" + sizeToString(static_cast<std::size_t>(time(NULL)));

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

        if(filesUploaded == 0)
        {
            handleError(400, res);
            return;
        }

        const std::string ok = "Files uploaded successfully.";
        res.setStatus(201);
        res.setHeader("Content-Type", "text/plain");
		res.setHeader("Content-Length", sizeToString(ok.size()));
		res.setBody(ok);

    } // multipart/form-data

    if(contentType.find("application/x-www-form-urlencoded") != std::string::npos)
    {
        if(body.empty())
        {
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
		res.setHeader("Content-Length", sizeToString(out.size()));
		res.setBody(out);
		return;
    }
    if(loc.upload_store.empty())
    {
        handleError(400, res);
        return;
    }
    const std::string dest = loc.upload_store + "/upload_" + sizeToString(static_cast<std::size_t>(time(NULL)));
    std::ofstream out(dest.c_str(), std::ios::binary);

    if(!out.is_open())
    {
        handleError(500, res);
        return ;
    }
    out.write(body.c_str(), static_cast<std::streamsize>(body.size()));
    out.close();

    const std::string ok = "Uploaded successfully.";
	res.setStatus(201);
	res.setHeader("Content-Type", "text/plain");
	res.setHeader("Content-Length", sizeToString(ok.size()));
	res.setBody(ok);
}






void RequestHandler::handleGet(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res, std::string &matched, bool withBody)
{
    // std::string relativePath = req.getPath().substr(loc.path.size());
    std::string relativePath = req.uri().substr(matched.size());
    std::string fullPath = loc.root + relativePath;


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
            res.setHeader("content-length", sizeToString(body.size()));
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
            res.setHeader("content-length", sizeToString(body.size()));
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
    res.setHeader("content-length", sizeToString(body.size()));
    if(withBody)
        res.setBody(body);
}

void RequestHandler::handleHead(const HttpRequest &req, const LocationConfig &loc, HttpResponse &res)
{
    handleGet(req, loc, res,Empty, false);
}

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

std::string RequestHandler::buildDirectoryListing(const std::string &path, const std::string &url) const
{

    DIR *dir = opendir(path.c_str());
    if(!dir)
    return Empty;

std::cout << "------------------> " << path << " <------------\n";
    std::ostringstream body;
    body << "<html><head><title>Index of " << url << "</title></head>"
		 << "<body><h1>Index of " << url << "</h1><hr><pre>";

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        const std::string name = entry->d_name;
        std::cout << "FILE : "<< name << std::endl;
        body << "<a href=\"" << url;
        if(url.empty() || url[url.size() - 1] != '/')
            body << '/';
        body << name << "\">" << name << "</a>\n";
    }
    closedir(dir);
    body << "</pre><hr></body></html>";
    return body.str();

}


bool RequestHandler::isDirectory(const std::string &path) const
{
    struct stat st;
    if(stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

std::string RequestHandler::readFile(const std::string &path, bool &ok) const{
    std::ifstream file(path.c_str(), std::ios::binary);
    if(!file.is_open())
        return Empty;

    std::ostringstream ss;
    ss << file.rdbuf();
    ok = true;
    return ss.str();
}

bool RequestHandler::fileExists(const std::string& path) const{
    struct stat st;
    if(stat(path.c_str(), &st) != 0)
        return false;
    return S_ISREG(st.st_mode);
}

void RequestHandler::handleError(int status, HttpResponse &res)
{
    res.clear();
    res.setStatus(status);
    writeErrorBody(status, res);
}

void RequestHandler::writeErrorBody(int code, HttpResponse &res) const
{
    const std::map<int, std::string>::const_iterator it = config_.error_pages.find(code);
    if(it != config_.error_pages.end())
    {
        bool ok = false;
        std::string body = readFile(it->second, ok);
        if(ok)
        {
            res.setHeader("content-type", mimeTypes(it->second));
            res.setHeader("content-length", sizeToString(body.size()));
            res.setBody(body);
            return ;
        }
    }

    std::ostringstream html;
    html << "<html><body><h1>" << code << " " << res.statusReason()
     << "</h1></body></html>";
    
    const std::string body = html.str();
    res.setHeader("content-type", "text/html");
    res.setHeader("content-length", sizeToString(body.size()));
    res.setBody(body);
}


std::string RequestHandler::mimeTypes(const std::string &path) const 
{
    const std::size_t dot = path.rfind('.');
    if(dot == std::string::npos || dot < path.find_last_of('/'))
        return "application/octet-stream";

    std::string ext = path.substr(dot + 1);

    for (std::size_t i = 0; i < ext.size(); ++i)
        ext[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i])));

    const std::size_t sz = sizeof(MimeTable) / sizeof(MimeTable[0]);
    for(std::size_t i = 0; i < sz; ++i)
        if(MimeTable[i].ext == ext)
            return MimeTable[i].type;

    return "application/octet-stream";
}
