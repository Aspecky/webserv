#pragma once

#include <map>
#include <string>

class HttpRequest {
  public:
	HttpRequest();
	~HttpRequest();

	void reset();

	const std::string &method() const;
	void			   method(const std::string &method);

	const std::string &path() const;
	void			   path(const std::string &path);

	const std::string &query() const;
	void			   query(const std::string &query);

	const std::string &version() const;
	void			   version(const std::string &version);

	const std::string &body() const;
	void			   body(const std::string &body);

	bool			   hasHeader(const std::string &name) const;
	const std::string &header(const std::string &name) const;
	void header(const std::string &name, const std::string &value);
	const std::map<std::string, std::string> &headers() const;

  private:
	HttpRequest(const HttpRequest &);
	HttpRequest &operator=(const HttpRequest &);

	std::string						   method_;
	std::string						   path_;
	std::string						   query_;
	std::string						   version_;
	std::map<std::string, std::string> headers_;
	std::string						   body_;
};
