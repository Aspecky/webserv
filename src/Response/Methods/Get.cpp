
#include "Response/HttpResponse.hpp"
#include <iostream>
// #include "../../includes/Client.hpp"




//  -----------------------> [ Handle Get Request ] <-------------------------------- //

void HttpResponse::_handleGet(const HttpRequest &req, const LocationConfig &loc)
{
    // std::string relativePath = req.uri().substr(loc.path.size());
    std::string fullPath = loc.root;


    // std::cout << BLUE << fullPath << RESET<< std::endl;

  // Is Directory
    if(_isDirectory(fullPath))
    {
        // std::cout << RED << "HERE" << RESET <<  std::endl;
        std::string indexPath = fullPath;
        if(indexPath[indexPath.size() -1] != '/')
          indexPath += '/';
        indexPath += loc.index.empty() ? "index.html" : loc.index;
        // std::cout << BLUE << indexPath << RESET<< std::endl;


        if(_isFileExists(indexPath))
        {
          _readAndSet(indexPath);
          _reqWithBody = true;
          return;
        }
        if(loc.directory_listing)
        {
          if(_reqWithBody)
            _body = _buildDirectoryListing(fullPath, req.uri());

          _statusCode = 200;
          _statusText = _statusMsg(_statusCode);
          _headers["Content-Type"] = "text/html";
          _headers["Content-Length"] = _toString(_body.size());
          _reqWithBody = true;
          return;
        }

        _buildError(403);
          return;
    }
    if(!_isFileExists(fullPath))
    {
      // std::cout << RED << "HERE" << RESET <<  std::endl;
      _buildError(404);
      return;
    }

    
    if(_reqWithBody)
      _body                         = _readFile(fullPath);

    _statusCode                   = 200;
    _statusText                   = _statusMsg(_statusCode);
    _headers["Content-Type"]      = _getMimeType(fullPath);
    _headers["Content-Length"]    = _toString(_body.size());

    _reqWithBody = true;

}
//  -----------------------> [ end ] <-------------------------------- //


//  -----------------------> [ handel Head Method ] <-------------------------------- //

void HttpResponse::_handleHead(const HttpRequest &req, const LocationConfig &loc)
{
    _reqWithBody = false;
    _handleGet(req, loc);
}
//  -----------------------> [ end ] <-------------------------------- //
