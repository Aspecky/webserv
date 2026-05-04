#pragma once

#include "Config/Config.hpp"
#include "Core/Client.hpp"
#include "Core/Server.hpp"
#include <cstddef>
#include <map>
#include <sys/poll.h>
#include <vector>

#define BACKLOG 10

class Reactor {
  public:
	Reactor(const Config &config);
	~Reactor();

	void run();

  private:
	Reactor(const Reactor &);
	Reactor &operator=(const Reactor &);

	void destroy_();

	std::vector<pollfd> pollFds_;

	std::map<int, Server *> servers_;
	std::map<int, Client *> clients_;


	void acceptConnection_(Server &server);
	void disconnectClient_(size_t &idx);
	void handleRead_(size_t &i);
	void handleClientWrite_(size_t &i);
};
