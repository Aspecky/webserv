#include "Core/Reactor.hpp"
#include "Config/Config.hpp"
#include "Config/ServerConfig.hpp"
#include "Core/Client.hpp"
#include "Core/Server.hpp"
#include <cstdio>
#include <exception>
#include <iostream>
#include <map>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

Reactor::Reactor(const Config &config)
{
	try {
		const std::vector<ServerConfig> &servers = config.servers();

		for (size_t i = 0; i < servers.size(); i++) {
			const ServerConfig &serverConfig = servers[i];
			Server			   *server		 = new Server(serverConfig);
			int					sockFd		 = server->socketFd();

			servers_[sockFd] = server;

			struct pollfd pfd = {};
			pfd.fd			  = sockFd;
			pfd.events		  = POLLIN;
			pollFds_.push_back(pfd);

			std::cout << "Listening on port " << server->config().port
					  << std::endl;
		}
	}
	catch (...) {
		destroy_();
		throw;
	}
}

void Reactor::destroy_()
{
	for (std::map<int, Server *>::iterator it = servers_.begin();
		 it != servers_.end(); ++it) {
		delete it->second;
	}

	for (std::map<int, Client *>::iterator it = clients_.begin();
		 it != clients_.end(); ++it) {
		delete it->second;
	}
}

Reactor::~Reactor()
{
	destroy_();
}

void Reactor::acceptConnection_(Server &server)
{
	Client *client = server.acceptClient();
	int		fd	   = client->socket();

	try {
		clients_[fd] = client;

		struct pollfd pfd = {};
		pfd.fd			  = fd;
		pfd.events		  = POLLIN;

		pollFds_.push_back(pfd);
	}
	catch (...) {
		clients_.erase(fd);
		delete client;
		throw;
	}

	std::cout << "New client" << std::endl;
}

void Reactor::disconnectClient_(size_t &idx)
{
	struct pollfd pfd = pollFds_[idx];

	pollFds_[idx] = pollFds_.back();
	pollFds_.pop_back();
	--idx;

	Client *client = clients_[pfd.fd];
	clients_.erase(pfd.fd);
	delete client;

	std::cout << "Client disconnected\n";
}

void Reactor::handleRead_(size_t &i)
{
	struct pollfd &pfd = pollFds_[i];

	std::map<int, Server *>::iterator it = servers_.find(pfd.fd);

	if (it != servers_.end()) {
		try {
			acceptConnection_(*it->second);
		}
		catch (std::exception &e) {
			std::cout << "Couldn't accept client: " << e.what() << std::endl;
		}
		return;
	}

	Client &client = *clients_[pfd.fd];

	char buf[BUFSIZ];
	long nbytes = recv(pfd.fd, static_cast<void *>(buf), sizeof(buf), 0);

	if (nbytes <= 0) {
		disconnectClient_(i);
		return;
	}

	client.onReceive(static_cast<char *>(buf), static_cast<size_t>(nbytes));

	if (client.hasResponse()) {
		pfd.events = POLLOUT;
	}
}

void Reactor::handleClientWrite_(size_t &i)
{
	struct pollfd &pfd	  = pollFds_[i];
	Client		  &client = *clients_[pfd.fd];

	long nbytes =
		send(pfd.fd, client.responseData(), client.responseSize(), 0);

	if (nbytes <= 0) {
		disconnectClient_(i);
		return;
	}

	client.consumeResponse(static_cast<size_t>(nbytes));

	if (client.shouldClose()) {
		disconnectClient_(i);
	}
	else if (client.responseSize() == 0) {
		pfd.events = POLLIN;
	}
}

void Reactor::run()
{
	while (true) {
		int pollCount = poll(&pollFds_[0], pollFds_.size(), -1);

		for (size_t i = 0; i < pollFds_.size() && pollCount > 0; ++i) {
			struct pollfd &pfd = pollFds_[i];

			if (!pfd.revents) {
				continue;
			}

			--pollCount;

			size_t idx = i;

			if (pfd.revents & POLLIN) {
				handleRead_(i);
			}

			if (idx == i && (pollFds_[i].revents & POLLOUT)) {
				handleClientWrite_(i);
			}
		}
	}
}
