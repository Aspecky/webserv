#ifndef HTTPRESPONSEWRITER_HPP
#define HTTPRESPONSEWRITER_HPP

#include <string>

class HttpResponse;

// Pure function from HttpResponse to wire bytes.
//
// Stateless. Appends to a caller-owned buffer to avoid a second copy. Does
// not call send(); that is still the Reactor's job.
//
// Responsibilities:
//   - status line, CRLF separators, blank line before body
//   - fill in Content-Length from body().size() if the handler did not set it
//   - apply Connection header based on `closeConnection`
struct HttpResponseWriter {
	static void serialize(const HttpResponse &res,
						  std::string		 &out,
						  bool				  closeConnection = false);
};

#endif
