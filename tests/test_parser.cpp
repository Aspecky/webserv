#include "Http/HttpParser.hpp"
#include "lest.hpp"
#include <string>

lest::tests parserSpec;

// MARK: parseRequestLine - method

lest_CASE(parserSpec, "parseRequestLine parses GET method")
{
	std::string s = "GET / HTTP/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(!p.hasError());
	lest_EXPECT(p.request().method() == "GET");
	lest_EXPECT(p.request().uri() == "/");
	lest_EXPECT(p.request().version() == "HTTP/1.1");
}

lest_CASE(parserSpec, "parseRequestLine parses POST method")
{
	std::string s = "POST /foo HTTP/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(!p.hasError());
	lest_EXPECT(p.request().method() == "POST");
}

lest_CASE(parserSpec, "parseRequestLine rejects empty method")
{
	std::string s = " / HTTP/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(p.hasError());
}

// MARK: parseRequestLine - origin-form request-target

lest_CASE(parserSpec, "parseRequestLine parses root path")
{
	std::string s = "GET / HTTP/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(!p.hasError());
	lest_EXPECT(p.request().uri() == "/");
}

lest_CASE(parserSpec, "parseRequestLine parses single segment path")
{
	std::string s = "GET /foo HTTP/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(!p.hasError());
	lest_EXPECT(p.request().uri() == "/foo");
}

lest_CASE(parserSpec, "parseRequestLine parses multi segment path")
{
	std::string s = "GET /foo/bar/baz HTTP/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(!p.hasError());
	lest_EXPECT(p.request().uri() == "/foo/bar/baz");
}

lest_CASE(parserSpec, "parseRequestLine parses path with query")
{
	std::string s = "GET /search?q=hello&lang=en HTTP/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(!p.hasError());
	lest_EXPECT(p.request().uri() == "/search?q=hello&lang=en");
}

lest_CASE(parserSpec, "parseRequestLine parses path with pct-encoded")
{
	std::string s = "GET /path%20with%20spaces HTTP/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(!p.hasError());
	lest_EXPECT(p.request().uri() == "/path%20with%20spaces");
}

lest_CASE(parserSpec, "parseRequestLine rejects non-slash target")
{
	std::string s = "GET foo HTTP/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(p.hasError());
}

// MARK: parseRequestLine - HTTP version

lest_CASE(parserSpec, "parseRequestLine parses HTTP/1.1 version")
{
	std::string s = "GET / HTTP/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(!p.hasError());
	lest_EXPECT(p.request().version() == "HTTP/1.1");
}

lest_CASE(parserSpec, "parseRequestLine parses HTTP/1.0 version")
{
	std::string s = "GET / HTTP/1.0\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(!p.hasError());
	lest_EXPECT(p.request().version() == "HTTP/1.0");
}

lest_CASE(parserSpec, "parseRequestLine rejects missing version")
{
	std::string s = "GET / \r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(p.hasError());
}

lest_CASE(parserSpec, "parseRequestLine rejects malformed version")
{
	std::string s = "GET / HTTPS/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(p.hasError());
}

// MARK: parseRequestLine - structure errors

lest_CASE(parserSpec, "parseRequestLine rejects missing SP after method")
{
	std::string s = "GET/ HTTP/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(p.hasError());
}

lest_CASE(parserSpec, "parseRequestLine rejects missing SP after target")
{
	std::string s = "GET /HTTP/1.1\r\n\r\n";
	HttpParser  p;
	p.feed(s.data(), s.size());
	lest_EXPECT(p.hasError());
}
