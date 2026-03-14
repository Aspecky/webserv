#include "Core/Reactor.hpp"
#include "Config/Config.hpp"
#include "Config/ServerConfig.hpp"
#include "Core/Client.hpp"
#include "Core/Server.hpp"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

Reactor::Reactor(const Config &config)
{
	const std::vector<ServerConfig>			 &servers = config.servers();
	std::vector<ServerConfig>::const_iterator it;

	for (it = servers.begin(); it != servers.end(); ++it) {
		int sockFd = socket(PF_INET, SOCK_STREAM, 0);

		int yes = 1;
		setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

		struct sockaddr_in addr = {};
		addr.sin_family			= PF_INET;
		addr.sin_port			= htons(it->port);
		addr.sin_addr.s_addr	= INADDR_ANY;

		if (bind(sockFd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
			std::cout << "bind: " << strerror(errno) << "\n";
			return;
		}

		if (listen(sockFd, BACKLOG) == -1) {
			return;
		}

		Server *server = new Server(sockFd);
		servers_.push_back(server);
		socketFdToServer_[sockFd] = server;

		struct pollfd pfd = {};
		pfd.fd			  = sockFd;
		pfd.events		  = POLLIN;
		pollFds_.push_back(pfd);
	}
}

Reactor::~Reactor()
{
}

void Reactor::acceptConnection_(Server &server)
{
	int fd = accept(server.socket(), NULL, NULL);

	Client *client = new Client(fd);

	clients_.push_back(client);
	socketFdToClient_[fd] = client;

	struct pollfd pfd = {};
	pfd.fd			  = fd;
	pfd.events		  = POLLIN;
	pollFds_.push_back(pfd);
}

void Reactor::handleClient_(Client &client, size_t &idx)
{
	int	 fd = client.socket();
	char buf[BUFSIZ];

	long nbytes = recv(fd, static_cast<void *>(buf), BUFSIZ, 0);

	if (nbytes <= 0) {
		if (nbytes == 0) {
			std::cout << "Client disconnected\n";
		}

		close(fd);
		pollFds_[idx] = pollFds_.back();
		pollFds_.pop_back();
		--idx;

		return;
	}

	send(fd, static_cast<void *>(buf), static_cast<size_t>(nbytes), 0);
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

			std::map<int, Server *>::iterator it =
				socketFdToServer_.find(pfd.fd);

			if (it != socketFdToServer_.end()) {
				acceptConnection_(*it->second);
			}
			else {
				handleClient_(*socketFdToClient_[pfd.fd], i);
			}
		}
	}
}
