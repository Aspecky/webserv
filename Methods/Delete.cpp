#include "Response/HttpResponse.hpp"


//  -----------------------> [ Handle DELETE Request ] <-------------------------------- //

void HttpResponse::_handleDelete(const HttpRequest &req, const LocationConfig &loc)
{
  std::string relativePath = req.uri().substr(loc.path.size());
  std::string fullPath = loc.root + relativePath;

  if(!_isFileExists(fullPath))
  {
    _buildError(404);
    return;
  }
  
  if(remove(fullPath.c_str()) != 0)
  {
    _buildError(500);
    return;
  }

  _body                         = "Fiel deleted.";
  _statusCode                   = 200;
  _statusText                   = _statusMsg(_statusCode);
  _headers["content-Type"]      = "text/plain";
  _headers["content-Length"]    = _toString(_body.size());

}
//  -----------------------> [ end ] <-------------------------------- //
