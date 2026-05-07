#pragma once

namespace status_codes
{
// 1xx Informational
const int CONTINUE			  = 100;
const int SWITCHING_PROTOCOLS = 101;
const int PROCESSING		  = 102;
const int EARLY_HINTS		  = 103;

// 2xx Success
const int OK			  = 200;
const int CREATED		  = 201;
const int ACCEPTED		  = 202;
const int NO_CONTENT	  = 204;
const int PARTIAL_CONTENT = 206;

// 3xx Redirection
const int SPECIAL_RESPONSE	 = 300;
const int MOVED_PERMANENTLY	 = 301;
const int MOVED_TEMPORARILY	 = 302;
const int SEE_OTHER			 = 303;
const int NOT_MODIFIED		 = 304;
const int TEMPORARY_REDIRECT = 307;
const int PERMANENT_REDIRECT = 308;

// 4xx Client Error
const int BAD_REQUEST			   = 400;
const int UNAUTHORIZED			   = 401;
const int FORBIDDEN				   = 403;
const int NOT_FOUND				   = 404;
const int NOT_ALLOWED			   = 405;
const int PROXY_AUTH_REQUIRED	   = 407;
const int REQUEST_TIME_OUT		   = 408;
const int CONFLICT				   = 409;
const int LENGTH_REQUIRED		   = 411;
const int PRECONDITION_FAILED	   = 412;
const int REQUEST_ENTITY_TOO_LARGE = 413;
const int REQUEST_URI_TOO_LARGE	   = 414;
const int UNSUPPORTED_MEDIA_TYPE   = 415;
const int RANGE_NOT_SATISFIABLE	   = 416;
const int MISDIRECTED_REQUEST	   = 421;
const int TOO_MANY_REQUESTS		   = 429;

// 5xx Server Error
const int INTERNAL_SERVER_ERROR = 500;
const int NOT_IMPLEMENTED		= 501;
const int BAD_GATEWAY			= 502;
const int SERVICE_UNAVAILABLE	= 503;
const int GATEWAY_TIME_OUT		= 504;
const int VERSION_NOT_SUPPORTED = 505;
const int INSUFFICIENT_STORAGE	= 507;
} // namespace status_codes
