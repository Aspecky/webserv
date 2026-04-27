#include "Http/Grammar.hpp"
#include "Http/Reader.hpp"

grammar::Rule::~Rule()
{
}

// MARK: HTTP

// token = 1*tchar
bool grammar::http::Token::operator()(Reader &r) const
{
	ReaderGuard guard(r);

	if (!r.consumeWhile(grammar::http::tchar)) {
		return false;
	}

	guard.commit();
	return true;
}

// MARK: URI

// pct-encoded = "%" HEXDIG HEXDIG
bool grammar::uri::PctEncoded::operator()(Reader &r) const
{
	ReaderGuard guard(r);

	if (!r.consume('%')) {
		return false;
	}
	if (!r.consumeOne(grammar::uri::hexdig)) {
		return false;
	}
	if (!r.consumeOne(grammar::uri::hexdig)) {
		return false;
	}

	guard.commit();
	return true;
}

// pchar = unreserved / pct-encoded / sub-delims / ":" / "@"
bool grammar::uri::Pchar::operator()(Reader &r) const
{
	ReaderGuard guard(r);

	bool matched = r.consumeOne(grammar::uri::unreserved) || PctEncoded()(r) ||
				   r.consumeOne(grammar::uri::subDelim) || r.consume(':') ||
				   r.consume('@');

	if (!matched) {
		return false;
	}

	guard.commit();
	return true;
}

// segment = *pchar
bool grammar::uri::Segment::operator()(Reader &r) const
{
	r.consumeWhileRule(Pchar());
	return true;
}

// absolute-path  = 1*( "/" segment )
bool grammar::uri::AbsolutePath::operator()(Reader &r) const
{
	ReaderGuard guard(r);

	if (!r.consume('/')) {
		return false;
	}
	r.consumeRule(Segment());

	while (r.consume('/')) {
		r.consumeRule(Segment());
	}

	guard.commit();
	return true;
}

// query = *( pchar / "/" / "?" )
bool grammar::uri::Query::operator()(Reader &r) const
{
	while (r.consumeRule(Pchar()) || r.consume('/') || r.consume('?')) {
	}
	return true;
}

// origin-form = absolute-path [ "?" query ]
bool grammar::uri::OriginForm::operator()(Reader &r) const
{
	ReaderGuard guard(r);

	if (!r.consumeRule(AbsolutePath())) {
		return false;
	}

	if (r.consume('?')) {
		r.consumeRule(Query());
	}

	guard.commit();
	return true;
}

// HTTP-name = %x48.54.54.50 ; HTTP
// HTTP-version = HTTP-name "/" DIGIT "." DIGIT
bool grammar::http::HttpVersion::operator()(Reader &r) const
{
	ReaderGuard guard(r);

	bool matched = r.consumeLiteral("HTTP") && r.consume('/') &&
				   r.consumeOne(abnf::digit) && r.consume('.') &&
				   r.consumeOne(abnf::digit);

	if (!matched) {
		return false;
	}

	guard.commit();
	return true;
}

// OWS = *( SP / HTAB )
bool grammar::http::OWS::operator()(Reader &r) const
{
	while (r.consume(abnf::SP) || r.consume(abnf::HTAB)) {
	}

	return true;
}

// field-content = field-vchar [ 1*( SP / HTAB / field-vchar ) field-vchar ]
bool grammar::http::FieldContent::operator()(Reader &r) const
{
	if (!r.consumeOne(grammar::http::fieldVchar)) {
		return false;
	}

	{
		ReaderGuard guard(r);

		const char *start = r.pos;

		while (r.consume(abnf::SP) || r.consume(abnf::HTAB) ||
			   r.consumeOne(fieldVchar)) {
			;
		}

		if (r.pos - start >= 2 && fieldVchar(*(r.pos - 1))) {
			guard.commit();
		}
	}

	return true;
}

// field-value = *field-content
bool grammar::http::FieldValue::operator()(Reader &r) const
{
	r.consumeWhileRule(FieldContent());
	return true;
}

// message-body = *OCTET
bool grammar::http::MessageBody::operator()(Reader &r) const
{
	r.pos = r.end;
	return true;
}
