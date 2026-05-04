#pragma once

#include "Http/Reader.hpp"
#include <cstddef>

// TODO: Consider whether passing `char c` is a safe choice

namespace grammar
{

namespace abnf
{

const char SP	= ' ';
const char HTAB = '\t';

// ALPHA = %x41-5A / %x61-7A ; A-Z / a-z
inline bool alpha(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

// DIGIT =  %x30-39 ; 0-9~
inline bool digit(char c)
{
	return c >= '0' && c <= '9';
}

// HEXDIG = DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
inline bool hexdig(char c)
{
	return digit(c) || (c >= 'A' && c <= 'F');
}

// VCHAR =  %x21-7E ; visible (printing) characters
inline bool vchar(char c)
{
	return (c >= '!' && c <= '~');
}

} // namespace abnf

namespace uri
{

inline bool hexdig(char c)
{
	return abnf::hexdig(c) || (c >= 'a' && c <= 'f');
}

// unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
inline bool unreserved(char c)
{
	return abnf::alpha(c) || abnf::digit(c) || c == '-' || c == '.' ||
		   c == '_' || c == '~';
}

// sub-delims  = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
inline bool subDelim(char c)
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
		if (!r.consumeOne(hexdig)) {
			return 0;
		}
		if (!r.consumeOne(hexdig)) {
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

// absolute-path = 1*("/" segment)
struct AbsolutePath {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));
		if (!r.consume('/')) {
			return 0;
		}
		r.consumeRule(Segment());
		while (r.consume('/')) {
			r.consumeRule(Segment());
		}
		return r.pos;
	}
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

// origin-form = absolute-path ["?" query]
struct OriginForm {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (!r.consumeRule(AbsolutePath())) {
			return 0;
		}
		if (r.consume('?')) {
			r.consumeRule(Query());
		}

		return r.pos;
	}
};

} // namespace uri

namespace http
{

inline bool tchar(char c)
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

// HTTP-version = "HTTP" "/" DIGIT "." DIGIT
struct HttpVersion {
	const char *operator()(const char *pos, const char *end) const
	{
		Reader r(pos, static_cast<size_t>(end - pos));

		if (!r.consumeLiteral("HTTP")) {
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
inline bool obsText(char c)
{
	return static_cast<unsigned char>(c) >= 0x80;
}

inline bool fieldVchar(char c)
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

		while (!r.done() && (fieldVchar(r.peek()) || r.peek() == abnf::SP ||
							 r.peek() == abnf::HTAB)) {
			r.consume(r.peek());
		}
		if (r.pos - tail >= 2 && fieldVchar(*(r.pos - 1))) {
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
} // namespace grammar
