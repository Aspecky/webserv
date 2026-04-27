#include "Http/Reader.hpp"


// TODO: Consider whether passing `char c` is a safe choice

namespace grammar
{

struct Rule {
	Rule()
	{
	}
	virtual ~Rule();
	virtual bool operator()(Reader &r) const = 0;

  private:
	Rule(const Rule &other);
	Rule &operator=(Rule other);
};

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

struct PctEncoded : Rule {
	bool operator()(Reader &r) const;
};

struct Pchar : Rule {
	bool operator()(Reader &r) const;
};

struct Segment : Rule {
	bool operator()(Reader &r) const;
};

struct AbsolutePath : Rule {
	bool operator()(Reader &r) const;
};

struct Query : Rule {
	bool operator()(Reader &r) const;
};

struct OriginForm : Rule {
	bool operator()(Reader &r) const;
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

struct Token : Rule {
	bool operator()(Reader &r) const;
};

struct HttpVersion : Rule {
	bool operator()(Reader &r) const;
};

struct OWS : Rule {
	bool operator()(Reader &r) const;
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

struct FieldContent : Rule {
	bool operator()(Reader &r) const;
};

struct FieldValue : Rule {
	bool operator()(Reader &r) const;
};

// message-body = *OCTET
struct MessageBody : Rule {
	bool operator()(Reader &r) const;
};

} // namespace http
} // namespace grammar
