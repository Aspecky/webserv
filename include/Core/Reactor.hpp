#pragma once


#include "Config/ConfigParser.hpp"
#include "Core/Client.hpp"
#include "Core/Server.hpp"
#include <cstddef>
#include <map>
#include <sys/poll.h>
#include <vector>
	
#define BACKLOG 10

class Reactor {
  public:
	Reactor(const std::vector<ServerConfig> &config);
	~Reactor();

	void run();

  private:
	Reactor(const Reactor &);
	Reactor &operator=(const Reactor &);

	void new_();
	void destroy_();

	std::vector<pollfd> pollFds_;

	std::vector<Server *>	servers_;
	std::map<int, Server *> socketFdToServer_;

	std::vector<Client *>	clients_;
	std::map<int, Client *> socketFdToClient_;

	void acceptConnection_(Server &server);
	void disconnectClient_(size_t idx);
	void handleRead();
	void handleClient_(Client &client, size_t &idx);
};
