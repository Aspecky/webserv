#pragma once
#include <cstddef>

struct Reader {
	const char *start;
	const char *pos;
	const char *end;

	Reader(const char *data, size_t len);

	bool   done() const;
	size_t remaining() const;
	size_t consumed() const;
	char   peek() const;
	bool   contains(const char *s) const;

	bool consume(char c);
	bool consumeLiteral(const char *s);

	template <typename Predicate> bool consumeOne(Predicate pred)
	{
		if (done() || !pred(static_cast<unsigned char>(*pos))) {
			return false;
		}
		++pos;

		return true;
	}

	template <typename Rule> bool consumeRule(const Rule &rule)
	{
		const char *next = rule(pos, end);

		if (!next) {
			return false;
		}
		pos = next;

		return true;
	}

	template <typename Predicate> bool consumeWhile(const Predicate &pred)
	{
		const char *start = pos;

		while (!done() && pred(static_cast<unsigned char>(*pos))) {
			++pos;
		}

		return pos != start;
	}

	template <typename Rule> bool consumeWhileRule(const Rule &rule)
	{
		const char *start = pos;
		const char *next  = 0;

		while ((next = rule(pos, end))) {
			pos = next;
		}

		return pos != start;
	}
};
