// #ifndef HTTPRESPONSE_HPP
// #define HTTPRESPONSE_HPP

// #include "Config/ConfigTypes.hpp"
// #include "Http/HttpRequest.hpp"
// #include <map>
// #include <string>

// class HttpResponse {

//   private:

//       int                                   _statusCode;
//       std::string                           _statusText;
//       std::map<std::string, std::string>    _headers;
//       std::string                           _body;
//       bool                                  _reqWithBody;
      
//       // for building response with defferece methods
//       void _handleGet(const HttpRequest &req, const LocationConfig &loc);
//       void _handlePost(const HttpRequest &req, const LocationConfig &loc);
//       void _handleDelete(const HttpRequest &req, const LocationConfig &loc);
//       void _handleHead(const HttpRequest &req, const LocationConfig &loc);

//       // utility
//       std::string       _getMimeType(const std::string& path);
//       std::string       _readFile(const std::string& path);
//       std::string       _statusMsg(int code);
//       std::string       _buildDirectoryListing(const std::string& path, const std::string& url);
//       bool              _isDirectory(const std::string& path);
//       bool              _isFileExists(const std::string& path);
//       std::string       _toString(size_t code);
//       void              _readAndSet(std::string& path);


//   public:

//       HttpResponse();
//       void              buildResponse(const HttpRequest &req, const ServerConfig &con);
//       std::string       toString();

//       // MIME types map
//       static std::map<std::string, std::string> mimeTypes;
//       static bool mimeTypesInitialized;
//       static void initMimeTypes();
//       void _buildError(int code);

 
// };

// #endif
