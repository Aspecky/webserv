#include "Http/Reader.hpp"
#include <cstddef>

Reader::Reader(const char *data, size_t len)
	: start(data), pos(data), end(data + len)
{
}

bool Reader::done() const
{
	return pos >= end;
}

size_t Reader::remaining() const
{
	return static_cast<size_t>(end - pos);
}

size_t Reader::consumed() const
{
	return static_cast<size_t>(pos - start);
}

char Reader::peek() const
{
	return *pos;
}

bool Reader::consume(char c)
{
	if (done() || *pos != c) {
		return false;
	}
	++pos;

	return true;
}

bool Reader::consumeLiteral(const char *s)
{
	const char *p = pos;
	while (*s) {
		if (p >= end || *p != *s) {
			return false;
		}
		++p;
		++s;
	}
	pos = p;

	return true;
}

bool Reader::contains(const char *s) const
{
	for (const char *p = pos; p < end; ++p) {
		size_t j = 0;
		while (s[j] && (p + j) < end && p[j] == s[j]) {
			++j;
		}
		if (s[j] == '\0') {
			return true;
		}
	}

	return false;
}
