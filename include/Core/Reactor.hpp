#pragma once

#include "Core/Client.hpp"
#include "Core/Server.hpp"
#include <cstddef>
#include <map>
#include <sys/poll.h>
#include <vector>

class Reactor {
  public:
	Reactor();
	~Reactor();

	void run();

  private:
	Reactor(const Reactor &);
	Reactor &operator=(const Reactor &);

	std::vector<pollfd>		pollFds_;

	std::vector<Server *>	servers_;
	std::map<int, Server *> socketFdToServer_;
	
	std::vector<Client *>	clients_;
	std::map<int, Client *> socketFdToClient_;

	void acceptConnection_(Server &server);
	void handleClient_(Client &client, size_t &idx);
};
