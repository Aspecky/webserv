#pragma once

#include "Http/Reader.hpp"
#include <cstddef>

namespace grammar
{

namespace abnf
{

const char SP	= ' ';
const char HTAB = '\t';
const char CR	= '\r';
const char LF	= '\n';

// ALPHA = %x41-5A / %x61-7A ; A-Z / a-z
inline bool alpha(unsigned char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

// DIGIT =  %x30-39 ; 0-9
inline bool digit(unsigned char c)
{
	return c >= '0' && c <= '9';
}

// HEXDIG = DIGIT / "A" / "B" / "C" / "D" / "E" / "F" (case-insensitive)
inline bool hexdig(unsigned char c)
{
	return digit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

// VCHAR =  %x21-7E ; visible (printing) characters
inline bool vchar(unsigned char c)
{
	return (c >= '!' && c <= '~');
}

// OCTET = %x00-FF
inline bool octet(unsigned char /*c*/)
{
	return true;
}

// CRLF = CR LF
struct CRLF {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (!r.consume(CR)) {
			return 0;
		}
		if (!r.consume(LF)) {
			return 0;
		}

		return r.pos;
	}
};

} // namespace abnf

namespace uri
{

// unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
inline bool unreserved(unsigned char c)
{
	return abnf::alpha(c) || abnf::digit(c) || c == '-' || c == '.' ||
		   c == '_' || c == '~';
}

// sub-delims  = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
inline bool subDelim(unsigned char c)
{
	return c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' ||
		   c == ')' || c == '*' || c == '+' || c == ',' || c == ';' || c == '=';
}

// pct-encoded = "%" HEXDIG HEXDIG
struct PctEncoded {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (!r.consume('%')) {
			return 0;
		}
		if (!r.consumeOne(abnf::hexdig)) {
			return 0;
		}
		if (!r.consumeOne(abnf::hexdig)) {
			return 0;
		}
		return r.pos;
	}
};

// pchar = unreserved / pct-encoded / sub-delims / ":" / "@"
struct Pchar {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (r.consumeOne(unreserved) || r.consumeOne(subDelim) ||
			r.consume(':') || r.consume('@')) {
			return r.pos;
		}
		return PctEncoded()(pos, end);
	}
};

// segment = *pchar
struct Segment {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		r.consumeWhileRule(Pchar());
		return r.pos;
	}
};

// segment-nz = 1*pchar
struct SegmentNz {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (!r.consumeWhileRule(Pchar())) {
			return 0;
		}
		return r.pos;
	}
};

// "/" segment
struct SlashSegment {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (!r.consume('/')) {
			return 0;
		}
		if (!r.consumeRule(Segment())) {
			return 0;
		}
		return r.pos;
	}
};

// path-empty = 0<pchar>
struct PathEmpty {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		return r.pos;
	}
};

// path-abempty = *( "/" segment )
struct PathAbempty {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		r.consumeWhileRule(SlashSegment());
		return r.pos;
	}
};

// path-absolute = "/" [ segment-nz *( "/" segment ) ]
struct PathAbsolute {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (!r.consume('/')) {
			return 0;
		}
		r.consumeRule(SegmentNzPath());
		return r.pos;
	}

	// segment-nz *( "/" segment )
	struct SegmentNzPath {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (!r.consumeRule(SegmentNz())) {
				return 0;
			}
			r.consumeWhileRule(SlashSegment());
			return r.pos;
		}
	};
};

// path-rootless = segment-nz *( "/" segment )
struct PathRootless {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (!r.consumeRule(SegmentNz())) {
			return 0;
		}
		r.consumeWhileRule(SlashSegment());
		return r.pos;
	}
};

// h16 = 1*4HEXDIG
struct H16 {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (!r.consumeOne(abnf::hexdig)) {
			return 0;
		}
		r.consumeOne(abnf::hexdig);
		r.consumeOne(abnf::hexdig);
		r.consumeOne(abnf::hexdig);
		return r.pos;
	}
};

// dec-octet = DIGIT                 ; 0-9
//           / %x31-39 DIGIT         ; 10-99
//           / "1" 2DIGIT            ; 100-199
//           / "2" %x30-34 DIGIT     ; 200-249
//           / "25" %x30-35          ; 250-255
struct DecOctet {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (r.consumeRule(Range250())) {
			return r.pos;
		}
		if (r.consumeRule(Range200())) {
			return r.pos;
		}
		if (r.consumeRule(Range100())) {
			return r.pos;
		}
		if (r.consumeRule(Range10())) {
			return r.pos;
		}
		if (r.consumeOne(abnf::digit)) {
			return r.pos;
		}
		return 0;
	}

	static bool isDigit0to5(unsigned char c)
	{
		return c >= '0' && c <= '5';
	}
	static bool isDigit0to4(unsigned char c)
	{
		return c >= '0' && c <= '4';
	}
	static bool isDigit1to9(unsigned char c)
	{
		return c >= '1' && c <= '9';
	}

	// %x31-39 DIGIT
	struct Range10 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (!r.consumeOne(DecOctet::isDigit1to9)) {
				return 0;
			}
			if (!r.consumeOne(abnf::digit)) {
				return 0;
			}
			return r.pos;
		}
	};

	// "1" 2DIGIT
	struct Range100 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (!r.consume('1')) {
				return 0;
			}
			if (!r.consumeOne(abnf::digit)) {
				return 0;
			}
			if (!r.consumeOne(abnf::digit)) {
				return 0;
			}
			return r.pos;
		}
	};

	// "2" %x30-34 DIGIT
	struct Range200 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (!r.consume('2')) {
				return 0;
			}
			if (!r.consumeOne(DecOctet::isDigit0to4)) {
				return 0;
			}
			if (!r.consumeOne(abnf::digit)) {
				return 0;
			}
			return r.pos;
		}
	};

	// "25" %x30-35
	struct Range250 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (!r.consume('2')) {
				return 0;
			}
			if (!r.consume('5')) {
				return 0;
			}
			if (!r.consumeOne(DecOctet::isDigit0to5)) {
				return 0;
			}
			return r.pos;
		}
	};
};

// IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
struct IPv4address {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (!r.consumeRule(DecOctet())) {
			return 0;
		}
		if (!r.consume('.')) {
			return 0;
		}
		if (!r.consumeRule(DecOctet())) {
			return 0;
		}
		if (!r.consume('.')) {
			return 0;
		}
		if (!r.consumeRule(DecOctet())) {
			return 0;
		}
		if (!r.consume('.')) {
			return 0;
		}
		if (!r.consumeRule(DecOctet())) {
			return 0;
		}
		return r.pos;
	}
};

// ls32 = ( h16 ":" h16 ) / IPv4address
struct Ls32 {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (r.consumeRule(H16ColonH16())) {
			return r.pos;
		}
		if (!r.consumeRule(IPv4address())) {
			return 0;
		}
		return r.pos;
	}

	// h16 ":" h16
	struct H16ColonH16 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (!r.consumeRule(H16())) {
				return 0;
			}
			if (!r.consume(':')) {
				return 0;
			}
			if (!r.consumeRule(H16())) {
				return 0;
			}
			return r.pos;
		}
	};
};

// IPv6address =                            6( h16 ":" ) ls32
//             /                       "::" 5( h16 ":" ) ls32
//             / [               h16 ] "::" 4( h16 ":" ) ls32
//             / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
//             / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
//             / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
//             / [ *4( h16 ":" ) h16 ] "::"              ls32
//             / [ *5( h16 ":" ) h16 ] "::"              h16
//             / [ *6( h16 ":" ) h16 ] "::"
struct IPv6address {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (r.consumeRule(Alt1())) {
			return r.pos;
		}
		if (r.consumeRule(Alt2())) {
			return r.pos;
		}
		if (r.consumeRule(Alt3())) {
			return r.pos;
		}
		if (r.consumeRule(Alt4())) {
			return r.pos;
		}
		if (r.consumeRule(Alt5())) {
			return r.pos;
		}
		if (r.consumeRule(Alt6())) {
			return r.pos;
		}
		if (r.consumeRule(Alt7())) {
			return r.pos;
		}
		if (r.consumeRule(Alt8())) {
			return r.pos;
		}
		if (r.consumeRule(Alt9())) {
			return r.pos;
		}
		return 0;
	}

	// h16 ":"
	struct H16Colon {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (!r.consumeRule(H16())) {
				return 0;
			}
			if (!r.consume(':')) {
				return 0;
			}
			return r.pos;
		}
	};

	// 6( h16 ":" ) ls32
	struct Alt1 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(Ls32())) {
				return 0;
			}
			return r.pos;
		}
	};

	// "::" 5( h16 ":" ) ls32
	struct Alt2 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (!r.consumeLiteral("::")) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(Ls32())) {
				return 0;
			}
			return r.pos;
		}
	};

	// [ h16 ] "::" 4( h16 ":" ) ls32
	struct Alt3 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			r.consumeRule(H16());
			if (!r.consumeLiteral("::")) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(Ls32())) {
				return 0;
			}
			return r.pos;
		}
	};

	// [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
	struct Alt4 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			r.consumeRule(OptPrefix());
			if (!r.consumeLiteral("::")) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(Ls32())) {
				return 0;
			}
			return r.pos;
		}

		// *1( h16 ":" ) h16
		struct OptPrefix {
			const char *operator()(const char *pos, const char *end) const
			{
				Reader r(pos, static_cast<size_t>(end - pos));
				r.consumeRule(H16Colon());
				if (!r.consumeRule(H16())) {
					return 0;
				}
				return r.pos;
			}
		};
	};

	// [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
	struct Alt5 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			r.consumeRule(OptPrefix());
			if (!r.consumeLiteral("::")) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(Ls32())) {
				return 0;
			}
			return r.pos;
		}

		// *2( h16 ":" ) h16
		struct OptPrefix {
			const char *operator()(const char *pos, const char *end) const
			{
				Reader r(pos, static_cast<size_t>(end - pos));
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				if (!r.consumeRule(H16())) {
					return 0;
				}
				return r.pos;
			}
		};
	};

	// [ *3( h16 ":" ) h16 ] "::" h16 ":" ls32
	struct Alt6 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			r.consumeRule(OptPrefix());
			if (!r.consumeLiteral("::")) {
				return 0;
			}
			if (!r.consumeRule(H16Colon())) {
				return 0;
			}
			if (!r.consumeRule(Ls32())) {
				return 0;
			}
			return r.pos;
		}

		// *3( h16 ":" ) h16
		struct OptPrefix {
			const char *operator()(const char *pos, const char *end) const
			{
				Reader r(pos, static_cast<size_t>(end - pos));
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				if (!r.consumeRule(H16())) {
					return 0;
				}
				return r.pos;
			}
		};
	};

	// [ *4( h16 ":" ) h16 ] "::" ls32
	struct Alt7 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			r.consumeRule(OptPrefix());
			if (!r.consumeLiteral("::")) {
				return 0;
			}
			if (!r.consumeRule(Ls32())) {
				return 0;
			}
			return r.pos;
		}

		// *4( h16 ":" ) h16
		struct OptPrefix {
			const char *operator()(const char *pos, const char *end) const
			{
				Reader r(pos, static_cast<size_t>(end - pos));
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				if (!r.consumeRule(H16())) {
					return 0;
				}
				return r.pos;
			}
		};
	};

	// [ *5( h16 ":" ) h16 ] "::" h16
	struct Alt8 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			r.consumeRule(OptPrefix());
			if (!r.consumeLiteral("::")) {
				return 0;
			}
			if (!r.consumeRule(H16())) {
				return 0;
			}
			return r.pos;
		}

		// *5( h16 ":" ) h16
		struct OptPrefix {
			const char *operator()(const char *pos, const char *end) const
			{
				Reader r(pos, static_cast<size_t>(end - pos));
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				if (!r.consumeRule(H16())) {
					return 0;
				}
				return r.pos;
			}
		};
	};

	// [ *6( h16 ":" ) h16 ] "::"
	struct Alt9 {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			r.consumeRule(OptPrefix());
			if (!r.consumeLiteral("::")) {
				return 0;
			}
			return r.pos;
		}

		// *6( h16 ":" ) h16
		struct OptPrefix {
			const char *operator()(const char *pos, const char *end) const
			{
				Reader r(pos, static_cast<size_t>(end - pos));
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				r.consumeRule(H16Colon());
				if (!r.consumeRule(H16())) {
					return 0;
				}
				return r.pos;
			}
		};
	};
};

// reg-name = *( unreserved / pct-encoded / sub-delims )
struct RegName {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		while (r.consumeOne(unreserved) || r.consumeRule(PctEncoded()) ||
			   r.consumeOne(subDelim)) {
		}
		return r.pos;
	}
};

// IPvFuture = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
struct IPvFuture {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (!r.consume('v')) {
			return 0;
		}
		if (!r.consumeWhile(abnf::hexdig)) {
			return 0;
		}
		if (!r.consume('.')) {
			return 0;
		}
		if (!r.consumeWhileRule(UnreservedOrSubDelimOrColon())) {
			return 0;
		}
		return r.pos;
	}

	// unreserved / sub-delims / ":"
	struct UnreservedOrSubDelimOrColon {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (r.consumeOne(unreserved) || r.consumeOne(subDelim) ||
				r.consume(':')) {
				return r.pos;
			}
			return 0;
		}
	};
};

// IP-literal = "[" ( IPv6address / IPvFuture ) "]"
struct IPLiteral {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (!r.consume('[')) {
			return 0;
		}
		if (!r.consumeRule(IPv6address()) && !r.consumeRule(IPvFuture())) {
			return 0;
		}
		if (!r.consume(']')) {
			return 0;
		}
		return r.pos;
	}
};

// port = *DIGIT
struct Port {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		r.consumeWhile(abnf::digit);
		return r.pos;
	}
};

// userinfo = *( unreserved / pct-encoded / sub-delims / ":" )
struct Userinfo {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		while (r.consumeOne(unreserved) || r.consumeRule(PctEncoded()) ||
			   r.consumeOne(subDelim) || r.consume(':')) {
		}
		return r.pos;
	}
};

// host = IP-literal / IPv4address / reg-name
struct Host {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (r.consumeRule(IPLiteral())) {
			return r.pos;
		}
		if (r.consumeRule(IPv4address())) {
			return r.pos;
		}
		if (r.consumeRule(RegName())) {
			return r.pos;
		}
		return 0;
	}
};

// uri-host = host
struct UriHost {
	const char *operator()(const char *pos, const char *end) const
	{
		return Host()(pos, end);
	}
};

// authority = [ userinfo "@" ] host [ ":" port ]
struct Authority {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		r.consumeRule(UserinfoAt());
		if (!r.consumeRule(Host())) {
			return 0;
		}
		r.consumeRule(ColonPort());
		return r.pos;
	}

	// userinfo "@"
	struct UserinfoAt {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (!r.consumeRule(Userinfo())) {
				return 0;
			}
			if (!r.consume('@')) {
				return 0;
			}
			return r.pos;
		}
	};

	// ":" port
	struct ColonPort {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (!r.consume(':')) {
				return 0;
			}
			if (!r.consumeRule(Port())) {
				return 0;
			}
			return r.pos;
		}
	};
};

// query = *(pchar / "/" / "?")
struct Query {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		while (r.consumeRule(Pchar()) || r.consume('/') || r.consume('?')) {
		}

		return r.pos;
	}
};

// "?" query
struct QuestionQuery {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (!r.consume('?')) {
			return 0;
		}
		if (!r.consumeRule(Query())) {
			return 0;
		}
		return r.pos;
	}
};

// scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
struct Scheme {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (!r.consumeOne(abnf::alpha)) {
			return 0;
		}
		r.consumeWhile(schemeChar);
		return r.pos;
	}

	// ALPHA / DIGIT / "+" / "-" / "." 
	static bool schemeChar(unsigned char c)
	{
		return abnf::alpha(c) || abnf::digit(c) || c == '+' || c == '-' ||
			   c == '.';
	}
};

// hier-part = "//" authority path-abempty
//           / path-absolute
//           / path-rootless
//           / path-empty
struct HierPart {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (r.consumeRule(AuthorityPath())) {
			return r.pos;
		}
		if (r.consumeRule(PathAbsolute())) {
			return r.pos;
		}
		if (r.consumeRule(PathRootless())) {
			return r.pos;
		}
		if (!r.consumeRule(PathEmpty())) {
			return 0;
		}
		return r.pos;
	}

	// "//" authority path-abempty
	struct AuthorityPath {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));

			if (!r.consumeLiteral("//")) {
				return 0;
			}
			if (!r.consumeRule(Authority())) {
				return 0;
			}
			if (!r.consumeRule(PathAbempty())) {
				return 0;
			}
			return r.pos;
		}
	};
};

// absolute-URI = scheme ":" hier-part [ "?" query ]
struct AbsoluteURI {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (!r.consumeRule(Scheme())) {
			return 0;
		}
		if (!r.consume(':')) {
			return 0;
		}
		if (!r.consumeRule(HierPart())) {
			return 0;
		}
		r.consumeRule(QuestionQuery());
		return r.pos;
	}
};

} // namespace uri

namespace http
{

inline bool tchar(unsigned char c)
{
	return c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
		   c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
		   c == '^' || c == '_' || c == '`' || c == '|' || c == '~' ||
		   abnf::digit(c) || abnf::alpha(c);
}

// token = 1*tchar
struct Token {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (!r.consumeWhile(tchar)) {
			return 0;
		}

		return r.pos;
	}
};

// method = token
struct Method {
	const char *operator()(const char *pos, const char *end) const
	{
		return Token()(pos, end);
	}
};

// field-name = token
struct FieldName {
	const char *operator()(const char *pos, const char *end) const
	{
		return Token()(pos, end);
	}
};

// OWS = *(SP / HTAB)
struct OWS {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		while (r.consume(abnf::SP) || r.consume(abnf::HTAB)) {
		}

		return r.pos;
	}
};

// obs-text = %x80-FF
inline bool obsText(unsigned char c)
{
	return c >= 0x80 && c <= 0xff;
}

inline bool fieldVchar(unsigned char c)
{
	return abnf::vchar(c) || obsText(c);
}

// field-content = field-vchar [1*(SP / HTAB / field-vchar) field-vchar]
struct FieldContent {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (!r.consumeOne(fieldVchar)) {
			return 0;
		}

		const char *tail = r.pos;

		while (!r.done() && (fieldVchar(static_cast<unsigned char>(r.peek())) ||
							 r.peek() == abnf::SP || r.peek() == abnf::HTAB)) {
			r.consume(r.peek());
		}
		if (r.pos - tail >= 2 &&
			fieldVchar(static_cast<unsigned char>(*(r.pos - 1)))) {
			return r.pos;
		}

		return tail;
	}
};

// field-value = *field-content
struct FieldValue {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		r.consumeWhileRule(FieldContent());

		return r.pos;
	}
};

} // namespace http

namespace http11
{

// absolute-path = 1*("/" segment)
struct AbsolutePath {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (!r.consumeWhileRule(SlashSegment())) {
			return 0;
		}
		return r.pos;
	}

	// "/" segment
	struct SlashSegment {
		const char *operator()(const char *pos, const char *end) const
		{
			Reader r(pos, static_cast<size_t>(end - pos));
			if (!r.consume('/')) {
				return 0;
			}
			if (!r.consumeRule(uri::Segment())) {
				return 0;
			}
			return r.pos;
		}
	};
};

// origin-form = absolute-path ["?" query]
struct OriginForm {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (!r.consumeRule(AbsolutePath())) {
			return 0;
		}
		r.consumeRule(uri::QuestionQuery());

		return r.pos;
	}
};

// HTTP-name      = %x48.54.54.50 ; "HTTP"
// HTTP-version   = HTTP-name "/" DIGIT "." DIGIT
struct HttpVersion {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (!r.consumeLiteral("HTTP", true)) {
			return 0;
		}
		if (!r.consume('/')) {
			return 0;
		}
		if (!r.consumeOne(abnf::digit)) {
			return 0;
		}
		if (!r.consume('.')) {
			return 0;
		}
		if (!r.consumeOne(abnf::digit)) {
			return 0;
		}

		return r.pos;
	}
};

// absolute-form  = absolute-URI
struct AbsoluteForm {
	const char *operator()(const char *pos, const char *end) const
	{
		return uri::AbsoluteURI()(pos, end);
	}
};

// authority-form = uri-host ":" port
struct AuthorityForm {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (!r.consumeRule(uri::UriHost())) {
			return 0;
		}
		if (!r.consume(':')) {
			return 0;
		}
		if (!r.consumeRule(uri::Port())) {
			return 0;
		}
		return r.pos;
	}
};

// asterisk-form  = "*"
struct AsteriskForm {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (!r.consume('*')) {
			return 0;
		}
		return r.pos;
	}
};

// request-target = origin-form / absolute-form / authority-form / asterisk-form
struct RequestTarget {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (r.consumeRule(OriginForm())) {
			return r.pos;
		}
		if (r.consumeRule(AbsoluteForm())) {
			return r.pos;
		}
		if (r.consumeRule(AuthorityForm())) {
			return r.pos;
		}
		if (r.consumeRule(AsteriskForm())) {
			return r.pos;
		}
		return 0;
	}
};
} // namespace http11
} // namespace grammar
