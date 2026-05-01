#include "Response/HttpResponse.hpp"
#include <dirent.h>
#include <sstream>
#include <sys/stat.h>
#include <fstream>




// Done. Here's what the new _handlePost handles:

//   multipart/form-data (file uploads via <input type="file">)
//   - Extracts the boundary from Content-Type
//   - Iterates over each part, finds Content-Disposition to get the filename
//   - Strips path traversal (../, /) from filenames
//   - Saves each file to loc.upload_store
//   - Returns 201 Created

//   application/x-www-form-urlencoded (regular HTML forms)
//   - Parses key=value&key2=value2 pairs from the body
//   - Returns 200 OK with an HTML page listing the received fields

//   Raw body fallback (e.g. application/json, application/octet-stream)
//   - Saves body as a single file in upload_store
//   - Returns 201 Created

// recap: Working on a C++ HTTP webserver. Just implemented the POST handler to
//   support multipart file uploads and URL-encoded form data. Next: test the POST
//   handler with a real request. (disable recaps in /config)
//  -----------------------> [ Utilities ] <-------------------------------- //


std::string   HttpResponse::_buildDirectoryListing(const std::string& path, const std::string& url)
{
  DIR* dir = opendir(path.c_str());
  if(!dir)
    return "";

  std::ostringstream body;
  body << "<html><head><title> Index of " << url << "</title></head>";
  body << "<body><h1>Index of " << url << "</h1><hr><per>";


  struct dirent* entry;
  while((entry = readdir(dir)) != NULL)
  {
    std::string name = entry->d_name;
    body << "<a href=\"" << url ;
    if(url[url.size()-1] != '/')
      body << '/';
    body << name << "\">" << name << "</a>\n";
  }
  closedir(dir);
  body << "</pre><hr></body></html>";
  return body.str();
}



// this function return error based on status code
void HttpResponse::_buildError(int code) {
  _statusCode = code;
  _statusText = _statusMsg(code);
  _body = "<html><body><h1>" + _toString(code) + " " + _statusText + "</h1></body></html>";
  _headers["Content-Length"] = _toString(_body.size());
  _headers["Content-Type"] = "text/html";
}

std::string HttpResponse::_statusMsg(int code) {
    switch (code) 
    {
        case 200 : return "Ok";
        case 201 : return "Created";
        case 204 : return "No Content";
        case 301 : return "Moved Permanently";

        case 400 : return "bad request";
        case 401 : return "Unauthorized";
        case 403 : return "Forbidden";
        case 404 : return "Not Found";
        case 405 : return "Method Not Allowed";
        case 408 : return "Request Timeout";
        case 411 : return "Length Required";
        case 413 : return "Payload Too Large";
        case 414 : return "URI Too Long";
        case 415 : return "Unsupported Media Type";

        case 500 : return "Internal Server Error";
        case 501 : return "Not Implemented";
        default: return "Unknown";
    }
}


std::string HttpResponse::_toString(size_t code) {
  std::ostringstream ss;
  ss << code;
  return ss.str();
}

bool HttpResponse::_isDirectory(const std::string& path)
{
    struct stat st;
    if(stat(path.c_str(), &st) != 0)
      return false;
    return S_ISDIR(st.st_mode);
}

bool HttpResponse::_isFileExists(const std::string& path)
{
  struct stat st;
  return (stat(path.c_str(), &st) == 0);
}


void HttpResponse::_readAndSet(std::string& path)
{
  if(_reqWithBody)
    _body = _readFile(path);

  _statusCode = 200;
  _statusText = _statusMsg(_statusCode);
  _headers["Content-Length"] = _toString(_body.size());
  _headers["Content-Type"] = _getMimeType(path);
}

std::string HttpResponse::_readFile(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    if(!file.is_open()){
      _buildError(404);
      return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

// check Mime Type
std::string HttpResponse::_getMimeType(const std::string& path)
{
    initMimeTypes();
    
    size_t dotPos = path.rfind('.');
    if (dotPos == std::string::npos)
    {
        return "application/octet-stream"; // Default MIME type for unknown extensions
    }
    
    std::string extension = path.substr(dotPos + 1);
    
    for (size_t i = 0; i < extension.length(); ++i)
    {
        if (extension[i] >= 'A' && extension[i] <= 'Z')
            extension[i] += 32; // Convert to lowercase
    }
    
    std::map<std::string, std::string>::iterator it = mimeTypes.find(extension);
    if (it != mimeTypes.end())
    {
        return it->second;
    }
    
    return "application/octet-stream";
}


std::string HttpResponse::toString() 
{
  std::ostringstream response;

  response << "HTTP/1.1 " << _statusCode << " " << _statusText << "\r\n";
  
  for(std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it)
    response << it->first << ": " << it->second << "\r\n";
  
  response << "\r\n";
  response << _body;

  return response.str();
}



// Static member initialization
std::map<std::string, std::string> HttpResponse::mimeTypes;
bool HttpResponse::mimeTypesInitialized = false;

// Initialize MIME types map
void HttpResponse::initMimeTypes()
{
    if (HttpResponse::mimeTypesInitialized) return;
    
    // Text types
    mimeTypes["html"] = "text/html";
    mimeTypes["htm"] = "text/html";
    mimeTypes["shtml"] = "text/html";
    mimeTypes["css"] = "text/css";
    mimeTypes["xml"] = "text/xml";
    mimeTypes["txt"] = "text/plain";
    mimeTypes["jad"] = "text/vnd.sun.j2me.app-descriptor";
    mimeTypes["wml"] = "text/vnd.wap.wml";
    mimeTypes["htc"] = "text/x-component";
    mimeTypes["mml"] = "text/mathml";
    
    // Image types
    mimeTypes["gif"] = "image/gif";
    mimeTypes["jpeg"] = "image/jpeg";
    mimeTypes["jpg"] = "image/jpeg";
    mimeTypes["avif"] = "image/avif";
    mimeTypes["png"] = "image/png";
    mimeTypes["svg"] = "image/svg+xml";
    mimeTypes["svgz"] = "image/svg+xml";
    mimeTypes["tif"] = "image/tiff";
    mimeTypes["tiff"] = "image/tiff";
    mimeTypes["wbmp"] = "image/vnd.wap.wbmp";
    mimeTypes["webp"] = "image/webp";
    mimeTypes["ico"] = "image/x-icon";
    mimeTypes["jng"] = "image/x-jng";
    mimeTypes["bmp"] = "image/x-ms-bmp";
    
    // Font types
    mimeTypes["woff"] = "font/woff";
    mimeTypes["woff2"] = "font/woff2";
    
    // Application types
    mimeTypes["js"] = "application/javascript";
    mimeTypes["json"] = "application/json";
    mimeTypes["jar"] = "application/java-archive";
    mimeTypes["war"] = "application/java-archive";
    mimeTypes["ear"] = "application/java-archive";
    mimeTypes["hqx"] = "application/mac-binhex40";
    mimeTypes["doc"] = "application/msword";
    mimeTypes["pdf"] = "application/pdf";
    mimeTypes["ps"] = "application/postscript";
    mimeTypes["eps"] = "application/postscript";
    mimeTypes["ai"] = "application/postscript";
    mimeTypes["rtf"] = "application/rtf";
    mimeTypes["m3u8"] = "application/vnd.apple.mpegurl";
    mimeTypes["kml"] = "application/vnd.google-earth.kml+xml";
    mimeTypes["kmz"] = "application/vnd.google-earth.kmz";
    mimeTypes["xls"] = "application/vnd.ms-excel";
    mimeTypes["eot"] = "application/vnd.ms-fontobject";
    mimeTypes["ppt"] = "application/vnd.ms-powerpoint";
    mimeTypes["odg"] = "application/vnd.oasis.opendocument.graphics";
    mimeTypes["odp"] = "application/vnd.oasis.opendocument.presentation";
    mimeTypes["ods"] = "application/vnd.oasis.opendocument.spreadsheet";
    mimeTypes["odt"] = "application/vnd.oasis.opendocument.text";
    mimeTypes["pptx"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    mimeTypes["xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    mimeTypes["docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    mimeTypes["wmlc"] = "application/vnd.wap.wmlc";
    mimeTypes["wasm"] = "application/wasm";
    mimeTypes["7z"] = "application/x-7z-compressed";
    mimeTypes["cco"] = "application/x-cocoa";
    mimeTypes["jardiff"] = "application/x-java-archive-diff";
    mimeTypes["jnlp"] = "application/x-java-jnlp-file";
    mimeTypes["run"] = "application/x-makeself";
    mimeTypes["pl"] = "application/x-perl";
    mimeTypes["pm"] = "application/x-perl";
    mimeTypes["prc"] = "application/x-pilot";
    mimeTypes["pdb"] = "application/x-pilot";
    mimeTypes["rar"] = "application/x-rar-compressed";
    mimeTypes["rpm"] = "application/x-redhat-package-manager";
    mimeTypes["sea"] = "application/x-sea";
    mimeTypes["swf"] = "application/x-shockwave-flash";
    mimeTypes["sit"] = "application/x-stuffit";
    mimeTypes["tcl"] = "application/x-tcl";
    mimeTypes["tk"] = "application/x-tcl";
    mimeTypes["der"] = "application/x-x509-ca-cert";
    mimeTypes["pem"] = "application/x-x509-ca-cert";
    mimeTypes["crt"] = "application/x-x509-ca-cert";
    mimeTypes["xpi"] = "application/x-xpinstall";
    mimeTypes["xhtml"] = "application/xhtml+xml";
    mimeTypes["xspf"] = "application/xspf+xml";
    mimeTypes["zip"] = "application/zip";
    mimeTypes["bin"] = "application/octet-stream";
    mimeTypes["exe"] = "application/octet-stream";
    mimeTypes["dll"] = "application/octet-stream";
    mimeTypes["deb"] = "application/octet-stream";
    mimeTypes["dmg"] = "application/octet-stream";
    mimeTypes["iso"] = "application/octet-stream";
    mimeTypes["img"] = "application/octet-stream";
    mimeTypes["msi"] = "application/octet-stream";
    mimeTypes["msp"] = "application/octet-stream";
    mimeTypes["msm"] = "application/octet-stream";
    mimeTypes["atom"] = "application/atom+xml";
    mimeTypes["rss"] = "application/rss+xml";
    
    // Audio types
    mimeTypes["mid"] = "audio/midi";
    mimeTypes["midi"] = "audio/midi";
    mimeTypes["kar"] = "audio/midi";
    mimeTypes["mp3"] = "audio/mpeg";
    mimeTypes["ogg"] = "audio/ogg";
    mimeTypes["m4a"] = "audio/x-m4a";
    mimeTypes["ra"] = "audio/x-realaudio";
    
    // Video types
    mimeTypes["3gpp"] = "video/3gpp";
    mimeTypes["3gp"] = "video/3gpp";
    mimeTypes["ts"] = "video/mp2t";
    mimeTypes["mp4"] = "video/mp4";
    mimeTypes["mpeg"] = "video/mpeg";
    mimeTypes["mpg"] = "video/mpeg";
    mimeTypes["mov"] = "video/quicktime";
    mimeTypes["webm"] = "video/webm";
    mimeTypes["flv"] = "video/x-flv";
    mimeTypes["m4v"] = "video/x-m4v";
    mimeTypes["mng"] = "video/x-mng";
    mimeTypes["asx"] = "video/x-ms-asf";
    mimeTypes["asf"] = "video/x-ms-asf";
    mimeTypes["wmv"] = "video/x-ms-wmv";
    mimeTypes["avi"] = "video/x-msvideo";
    
    HttpResponse::mimeTypesInitialized = true;
}
