#include "Http/Grammar.hpp"
#include "Http/Reader.hpp"
#include "include/lest.hpp"

lest::tests rulesSpec;

// MARK: Token

lest_CASE(rulesSpec, "Token matches single tchar")
{
	const char *s = "a";
	Reader		r(s, 1);
	lest_EXPECT(grammar::http::Token()(r) == true);
	lest_EXPECT(r.consumed() == 1u);
}

lest_CASE(rulesSpec, "Token matches multiple tchars")
{
	const char *s = "Content-Type";
	Reader		r(s, 12);
	lest_EXPECT(grammar::http::Token()(r) == true);
	lest_EXPECT(r.consumed() == 12u);
}

lest_CASE(rulesSpec, "Token matches all symbol tchars")
{
	const char *s = "!#$%&'*+-.^_`|~";
	Reader		r(s, 15);
	lest_EXPECT(grammar::http::Token()(r) == true);
	lest_EXPECT(r.consumed() == 15u);
}

lest_CASE(rulesSpec, "Token stops at non-tchar")
{
	const char *s = "abc def";
	Reader		r(s, 7);
	lest_EXPECT(grammar::http::Token()(r) == true);
	lest_EXPECT(r.consumed() == 3u);
}

lest_CASE(rulesSpec, "Token rejects empty input")
{
	const char *s = "";
	Reader		r(s, 0);
	lest_EXPECT(grammar::http::Token()(r) == false);
	lest_EXPECT(r.consumed() == 0u);
}

lest_CASE(rulesSpec, "Token rejects leading space")
{
	const char *s = " abc";
	Reader		r(s, 4);
	lest_EXPECT(grammar::http::Token()(r) == false);
	lest_EXPECT(r.consumed() == 0u);
}

// MARK: AbsolutePath`

lest_CASE(rulesSpec, "AbsolutePath matches root slash")
{
	const char *s = "/";
	Reader		r(s, 1);
	lest_EXPECT(grammar::uri::AbsolutePath()(r) == true);
	lest_EXPECT(r.consumed() == 1u);
}

lest_CASE(rulesSpec, "AbsolutePath matches single segment")
{
	const char *s = "/foo";
	Reader		r(s, 4);
	lest_EXPECT(grammar::uri::AbsolutePath()(r) == true);
	lest_EXPECT(r.consumed() == 4u);
}

lest_CASE(rulesSpec, "AbsolutePath matches multiple segments")
{
	const char *s = "/foo/bar/baz";
	Reader		r(s, 12);
	lest_EXPECT(grammar::uri::AbsolutePath()(r) == true);
	lest_EXPECT(r.consumed() == 12u);
}

lest_CASE(rulesSpec, "AbsolutePath matches empty segments")
{
	const char *s = "/foo//bar";
	Reader		r(s, 9);
	lest_EXPECT(grammar::uri::AbsolutePath()(r) == true);
	lest_EXPECT(r.consumed() == 9u);
}

lest_CASE(rulesSpec, "AbsolutePath matches pct-encoded in segment")
{
	const char *s = "/foo%20bar";
	Reader		r(s, 10);
	lest_EXPECT(grammar::uri::AbsolutePath()(r) == true);
	lest_EXPECT(r.consumed() == 10u);
}

lest_CASE(rulesSpec, "AbsolutePath stops at query")
{
	const char *s = "/foo?query";
	Reader		r(s, 10);
	lest_EXPECT(grammar::uri::AbsolutePath()(r) == true);
	lest_EXPECT(r.consumed() == 4u);
}

lest_CASE(rulesSpec, "AbsolutePath rejects empty input")
{
	const char *s = "";
	Reader		r(s, 0);
	lest_EXPECT(grammar::uri::AbsolutePath()(r) == false);
	lest_EXPECT(r.consumed() == 0u);
}

lest_CASE(rulesSpec, "AbsolutePath rejects non-slash start")
{
	const char *s = "foo/bar";
	Reader		r(s, 7);
	lest_EXPECT(grammar::uri::AbsolutePath()(r) == false);
	lest_EXPECT(r.consumed() == 0u);
}

// MARK: Segment

lest_CASE(rulesSpec, "Segment matches empty input")
{
	const char *s = "";
	Reader		r(s, 0);
	lest_EXPECT(grammar::uri::Segment()(r) == true);
	lest_EXPECT(r.consumed() == 0u);
}

lest_CASE(rulesSpec, "Segment matches pchars")
{
	const char *s = "foo:bar@baz";
	Reader		r(s, 11);
	lest_EXPECT(grammar::uri::Segment()(r) == true);
	lest_EXPECT(r.consumed() == 11u);
}

lest_CASE(rulesSpec, "Segment matches pct-encoded")
{
	const char *s = "foo%2Fbar";
	Reader		r(s, 9);
	lest_EXPECT(grammar::uri::Segment()(r) == true);
	lest_EXPECT(r.consumed() == 9u);
}

lest_CASE(rulesSpec, "Segment stops before slash")
{
	const char *s = "foo/bar";
	Reader		r(s, 7);
	lest_EXPECT(grammar::uri::Segment()(r) == true);
	lest_EXPECT(r.consumed() == 3u);
}

lest_CASE(rulesSpec, "Segment stops before question mark")
{
	const char *s = "foo?bar";
	Reader		r(s, 7);
	lest_EXPECT(grammar::uri::Segment()(r) == true);
	lest_EXPECT(r.consumed() == 3u);
}

// MARK: PctEncoded

lest_CASE(rulesSpec, "PctEncoded matches %FF")
{
	const char *s = "%FF";
	Reader		r(s, 3);
	lest_EXPECT(grammar::uri::PctEncoded()(r) == true);
	lest_EXPECT(r.consumed() == 3u);
}

lest_CASE(rulesSpec, "PctEncoded rejects missing hex digit")
{
	const char *s = "%FZ";
	Reader		r(s, 3);
	lest_EXPECT(grammar::uri::PctEncoded()(r) == false);
	lest_EXPECT(r.consumed() == 0u);
}

// MARK: Pchar

lest_CASE(rulesSpec, "Pchar matches unreserved char")
{
	const char *s = "a";
	Reader		r(s, 1);
	lest_EXPECT(grammar::uri::Pchar()(r) == true);
}

lest_CASE(rulesSpec, "Pchar matches colon")
{
	const char *s = ":";
	Reader		r(s, 1);
	lest_EXPECT(grammar::uri::Pchar()(r) == true);
}

lest_CASE(rulesSpec, "Pchar rejects space")
{
	const char *s = " ";
	Reader		r(s, 1);
	lest_EXPECT(grammar::uri::Pchar()(r) == false);
}
