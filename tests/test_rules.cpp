#include "Http/Grammar.hpp"
#include "Http/Reader.hpp"
#include "include/lest.hpp"

lest::tests rulesSpec;

// MARK: CRLF

lest_CASE(rulesSpec, "CRLF matches CR LF")
{
	const char *s = "\r\n";
	Reader		r(s, 2);
	lest_EXPECT(r.consumeRule(grammar::abnf::CRLF()) == true);
	lest_EXPECT(r.consumed() == 2u);
}

lest_CASE(rulesSpec, "CRLF stops after CR LF with trailing data")
{
	const char *s = "\r\nfoo";
	Reader		r(s, 5);
	lest_EXPECT(r.consumeRule(grammar::abnf::CRLF()) == true);
	lest_EXPECT(r.consumed() == 2u);
}

lest_CASE(rulesSpec, "CRLF rejects LF only")
{
	const char *s = "\n";
	Reader		r(s, 1);
	lest_EXPECT(r.consumeRule(grammar::abnf::CRLF()) == false);
	lest_EXPECT(r.consumed() == 0u);
}

lest_CASE(rulesSpec, "CRLF rejects CR only")
{
	const char *s = "\r";
	Reader		r(s, 1);
	lest_EXPECT(r.consumeRule(grammar::abnf::CRLF()) == false);
	lest_EXPECT(r.consumed() == 0u);
}

lest_CASE(rulesSpec, "CRLF rejects empty input")
{
	const char *s = "";
	Reader		r(s, 0);
	lest_EXPECT(r.consumeRule(grammar::abnf::CRLF()) == false);
	lest_EXPECT(r.consumed() == 0u);
}

// MARK: Token

lest_CASE(rulesSpec, "Token matches single char")
{
	const char *s = "a";
	Reader		r(s, 1);
	lest_EXPECT(r.consumeRule(grammar::http::Token()) == true);
	lest_EXPECT(r.consumed() == 1u);
}

lest_CASE(rulesSpec, "Token matches header field name")
{
	const char *s = "Content-Type";
	Reader		r(s, 12);
	lest_EXPECT(r.consumeRule(grammar::http::Token()) == true);
	lest_EXPECT(r.consumed() == 12u);
}

lest_CASE(rulesSpec, "Token matches all valid symbols")
{
	const char *s = "!#$%&'*+-.^_`|~";
	Reader		r(s, 15);
	lest_EXPECT(r.consumeRule(grammar::http::Token()) == true);
	lest_EXPECT(r.consumed() == 15u);
}

lest_CASE(rulesSpec, "Token stops at SP")
{
	const char *s = "abc def";
	Reader		r(s, 7);
	lest_EXPECT(r.consumeRule(grammar::http::Token()) == true);
	lest_EXPECT(r.consumed() == 3u);
}

lest_CASE(rulesSpec, "Token stops at tspecial")
{
	const char *s = "abc:def";
	Reader		r(s, 7);
	lest_EXPECT(r.consumeRule(grammar::http::Token()) == true);
	lest_EXPECT(r.consumed() == 3u);
}

lest_CASE(rulesSpec, "Token rejects empty input")
{
	const char *s = "";
	Reader		r(s, 0);
	lest_EXPECT(r.consumeRule(grammar::http::Token()) == false);
	lest_EXPECT(r.consumed() == 0u);
}

lest_CASE(rulesSpec, "Token rejects leading tspecial")
{
	const char *s = ":abc";
	Reader		r(s, 4);
	lest_EXPECT(r.consumeRule(grammar::http::Token()) == false);
	lest_EXPECT(r.consumed() == 0u);
}

// MARK: IPv6address

// Alt1: 6( h16 ":" ) ls32  —  no "::"
lest_CASE(rulesSpec, "IPv6address Alt1 matches 6 h16: groups + h16:h16 ls32")
{
	// 2001:db8:85a3:0:0:8a2e:370:7334
	const char *s = "2001:db8:85a3:0:0:8a2e:370:7334";
	Reader		r(s, 31);
	lest_EXPECT(r.consumeRule(grammar::uri::IPv6address()) == true);
	lest_EXPECT(r.consumed() == 31u);
}

lest_CASE(rulesSpec, "IPv6address Alt1 matches 6 h16: groups + IPv4 ls32")
{
	// 2001:db8:85a3:0:0:8a2e:192.168.0.1
	const char *s = "2001:db8:85a3:0:0:8a2e:192.168.0.1";
	Reader		r(s, 34);
	lest_EXPECT(r.consumeRule(grammar::uri::IPv6address()) == true);
	lest_EXPECT(r.consumed() == 34u);
}

// Alt2: "::" 5( h16 ":" ) ls32
lest_CASE(rulesSpec, "IPv6address Alt2 matches :: + 5 h16: + IPv4 ls32")
{
	// ::1:2:3:4:5:1.2.3.4
	const char *s = "::1:2:3:4:5:1.2.3.4";
	Reader		r(s, 19);
	lest_EXPECT(r.consumeRule(grammar::uri::IPv6address()) == true);
	lest_EXPECT(r.consumed() == 19u);
}

// Alt3: [ h16 ] "::" 4( h16 ":" ) ls32  —  without optional prefix
lest_CASE(rulesSpec, "IPv6address Alt3 matches :: + 4 h16: + ls32 (no prefix)")
{
	// ::1:2:3:4:1.1.1.1
	const char *s = "::1:2:3:4:1.1.1.1";
	Reader		r(s, 17);
	lest_EXPECT(r.consumeRule(grammar::uri::IPv6address()) == true);
	lest_EXPECT(r.consumed() == 17u);
}

// Alt3: [ h16 ] "::" 4( h16 ":" ) ls32  —  with optional h16 prefix
lest_CASE(rulesSpec, "IPv6address Alt3 matches h16 :: + 4 h16: + ls32")
{
	// fe80::1:2:3:4:1.1.1.1
	const char *s = "fe80::1:2:3:4:1.1.1.1";
	Reader		r(s, 21);
	lest_EXPECT(r.consumeRule(grammar::uri::IPv6address()) == true);
	lest_EXPECT(r.consumed() == 21u);
}

// Alt8: [ *5( h16 ":" ) h16 ] "::" h16  —  loopback
lest_CASE(rulesSpec, "IPv6address Alt8 matches ::1 (loopback)")
{
	const char *s = "::1";
	Reader		r(s, 3);
	lest_EXPECT(r.consumeRule(grammar::uri::IPv6address()) == true);
	lest_EXPECT(r.consumed() == 3u);
}

// Alt9: [ *6( h16 ":" ) h16 ] "::"  —  unspecified
lest_CASE(rulesSpec, "IPv6address Alt9 matches :: (unspecified address)")
{
	const char *s = "::";
	Reader		r(s, 2);
	lest_EXPECT(r.consumeRule(grammar::uri::IPv6address()) == true);
	lest_EXPECT(r.consumed() == 2u);
}

lest_CASE(rulesSpec, "IPv6address stops at trailing non-address data")
{
	// ::1 followed by ] (IP-literal context)
	const char *s = "::1]rest";
	Reader		r(s, 8);
	lest_EXPECT(r.consumeRule(grammar::uri::IPv6address()) == true);
	lest_EXPECT(r.consumed() == 3u);
}

lest_CASE(rulesSpec, "IPv6address rejects empty input")
{
	const char *s = "";
	Reader		r(s, 0);
	lest_EXPECT(r.consumeRule(grammar::uri::IPv6address()) == false);
	lest_EXPECT(r.consumed() == 0u);
}

lest_CASE(rulesSpec, "IPv6address rejects invalid hex chars")
{
	const char *s = "gg::1";
	Reader		r(s, 5);
	lest_EXPECT(r.consumeRule(grammar::uri::IPv6address()) == false);
	lest_EXPECT(r.consumed() == 0u);
}

lest_CASE(rulesSpec, "IPv6address rejects too few groups without ::")
{
	// Only 5 h16: groups + ls32 — not enough for Alt1, no :: for others
	const char *s = "1:2:3:4:5:6:7";
	Reader		r(s, 13);
	lest_EXPECT(r.consumeRule(grammar::uri::IPv6address()) == false);
	lest_EXPECT(r.consumed() == 0u);
}
