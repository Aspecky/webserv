#include "Http/Helper.hpp"
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>

namespace RequestHelpers
{

struct MimeRow {
	const char *ext;
	const char *type;
};

const std::string Empty = "";

const MimeRow MimeTable[] = {
	{"html", "text/html"},		  {"htm", "text/html"},
	{"css", "text/css"},		  {"txt", "text/plain"},
	{"xml", "text/xml"},		  {"js", "application/javascript"},
	{"json", "application/json"}, {"pdf", "application/pdf"},
	{"zip", "application/zip"},	  {"png", "image/png"},
	{"gif", "image/gif"},		  {"jpg", "image/jpeg"},
	{"jpeg", "image/jpeg"},		  {"webp", "image/webp"},
	{"svg", "image/svg+xml"},	  {"ico", "image/x-icon"},
	{"mp3", "audio/mpeg"},		  {"mp4", "video/mp4"},
	{"webm", "video/webm"},		  {"woff", "font/woff"},
	{"woff2", "font/woff2"},
};

bool needGgi(std::string &path)
{

	std::size_t pos = path.rfind('.');
	if (pos == std::string::npos)
		return false;

	std::string ext = path.substr(pos);

	return (ext == ".py" || ext == ".bash");
}

std::string sizeToString(std::size_t n)
{
	std::ostringstream ss;
	ss << n;
	return ss.str();
}
std::string toLower(const std::string &s)
{
	std::string out(s);
	for (std::size_t i = 0; i < out.size(); ++i)
		out[i] =
			static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
	return out;
}

std::string extractBoundary(const std::string &contentType)
{
	const std::size_t pos = contentType.find("boundary=");
	if (pos == std::string::npos)
		return Empty;

	std::string boundary = contentType.substr(pos + 9);
	if (!boundary.empty() && boundary[0] == '"')
		boundary = boundary.substr(1, boundary.size() - 2);

	return boundary;
}

std::string extractFilename(const std::string &disposition)
{
	const std::size_t pos = disposition.find("filename=\"");
	if (pos == std::string::npos)
		return Empty;
	const std::size_t start = pos + 10;
	const std::size_t end	= disposition.find('"', start);
	if (end == std::string::npos)
		return Empty;
	return disposition.substr(start, end - start);
}

std::string getDisposition(const std::string &partHeaders)
{
	const std::string needle = "content-disposition:";
	std::string		  lower	 = toLower(partHeaders);
	std::size_t		  pos	 = lower.find(needle);

	if (pos == std::string::npos)
		return Empty;

	const std::size_t start = pos + needle.size();
	const std::size_t end	= lower.find("\r\n", start);
	std::string		  value = partHeaders.substr(
		start, (end == std::string::npos ? partHeaders.size() : end) - start);

	const std::size_t s = value.find_first_not_of(" \t");
	return (s == std::string::npos) ? Empty : value.substr(s);
}

std::string getContentType(const std::string &ext)
{
	const std::size_t sz = sizeof(MimeTable) / sizeof(MimeTable[0]);
	for (std::size_t i = 0; i < sz; ++i)
		if (MimeTable[i].ext == ext)
			return MimeTable[i].type;

	return "application/octet-stream";
}

static int hexVal(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

std::string urlDecode(const std::string &s)
{
	std::string out;
	out.reserve(s.size());
	for (std::size_t i = 0; i < s.size(); ++i) {
		if (s[i] == '%' && i + 2 < s.size()) {
			int hi = hexVal(s[i + 1]);
			int lo = hexVal(s[i + 2]);
			if (hi >= 0 && lo >= 0) {
				out += static_cast<char>((hi << 4) | lo);
				i += 2;
				continue;
			}
		}
		if (s[i] == '+')
			out += ' ';
		else
			out += s[i];
	}
	return out;
}

} // namespace RequestHelpers
