#pragma once

#include "Http/HttpParser.hpp"
#include "Http/HttpRequest.hpp"
#include <cstddef>
#include <string>

class Server;

class Client {
  public:
	Client(Server &server, int socketFd);
	~Client();

	int	 socket() const;
	void onReceive(const char *buf, size_t n);

	const HttpRequest &request() const;
	bool			   requestComplete() const;

	bool		hasResponse() const;
	const char *responseData() const;
	size_t		responseSize() const;
	void		consumeResponse(size_t n);

	bool shouldClose() const;

  private:
	Client(const Client &other);
	Client &operator=(const Client &other);

	Server	   &server_;
	int			socket_;
	HttpRequest request_;
	HttpParser	parser_;
	std::string writeBuffer_;
};
