#include "Core/Reactor.hpp"
#include "Config/Config.hpp"
#include "Config/ServerConfig.hpp"
#include "Core/Client.hpp"
#include "Core/Server.hpp"
#include "Http/HttpRequest.hpp"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
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

			servers_.push_back(server);
			socketFdToServer_[sockFd] = server;

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
	for (size_t i = 0; i < servers_.size(); ++i) {
		delete servers_[i];
	}

	for (size_t i = 0; i < clients_.size(); ++i) {
		delete clients_[i];
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
		clients_.push_back(client);
	}
	catch (...) {
		delete client;
		throw;
	}

	try {
		socketFdToClient_[fd] = client;

		struct pollfd pfd = {};
		pfd.fd			  = fd;
		pfd.events		  = POLLIN;

		pollFds_.push_back(pfd);
	}
	catch (...) {
		clients_.pop_back();
		socketFdToClient_.erase(fd);
		delete client;
		throw;
	}

	std::cout << "New client" << std::endl;
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

void Reactor::disconnectClient_(size_t idx)
{
	struct pollfd pfd = pollFds_[idx];

	pollFds_[idx] = pollFds_.back();
	pollFds_.pop_back();

	delete socketFdToClient_[pfd.fd];
	socketFdToClient_.erase(pfd.fd);
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

			// Check for recv readiness
			if (pfd.revents & POLLIN) {
				std::map<int, Server *>::iterator it =
					socketFdToServer_.find(pfd.fd);

				// If it's a server socket, accept new connection
				if (it != socketFdToServer_.end()) {
					try {
						acceptConnection_(*it->second);
					}
					catch (std::exception &e) {
						std::cout << "Couldn't accept client: " << e.what()
								  << std::endl;
					}
					continue;
				}

				// Otherwise it's a client socket
				Client &client = *socketFdToClient_[pfd.fd];

				char buf[BUFSIZ];
				long nbytes =
					recv(pfd.fd, static_cast<void *>(buf), sizeof(buf), 0);

				if (nbytes <= 0) {
					disconnectClient_(i);
					--i;

					if (nbytes == 0) {
						std::cout << "Client disconnected" << std::endl;
					}
					continue;
				}

				client.onReceive(buf, nbytes);

				if (client.request().hasError()) {
					disconnectClient_(i);
					--i;
					continue;
				}

				if (client.request().isComplete()) {
					const HttpRequest &req = client.request();

					std::cout << req.method() << " " << req.uri() << " "
							  << req.version() << "\n";

					const std::map<std::string, std::string> &headers =
						req.headers();

					std::map<std::string, std::string>::const_iterator it;

					for (it = headers.begin(); it != headers.end(); ++it) {
						std::cout << it->first << " " << it->second << "\n";
					}
				}
				if (client.hasResponse()) {
					pfd.events |= POLLOUT;
				}
			}

			// Check for send readiness
			if (pfd.events & POLLOUT) {
				Client &client = *socketFdToClient_[pfd.fd];

				long nbytes = send(pfd.fd, client.responseData(),
								   client.responseSize(), 0);

				if (nbytes <= 0) {
					disconnectClient_(i);
					--i;
					continue;
				}

				client.consumeResponse(nbytes);

				if (!client.hasResponse()) {
					pfd.events &= ~POLLOUT;
					// TODO: client.shouldClose()
				}
			}
		}
	}
}
