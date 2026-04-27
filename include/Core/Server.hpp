#pragma once

#include "Config/ConfigTypes.hpp"
class Client;

class Server {
  public:
	Server(const ServerConfig &config);
	~Server();

	int					socketFd() const;
	Client			   *acceptClient();
	const ServerConfig &config();

  private:
	Server(const Server &);
	Server &operator=(const Server &);

	const ServerConfig &config_;
	int					socketFd_;
};
