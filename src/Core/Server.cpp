#include "Core/Server.hpp"
#include "Config/ServerConfig.hpp"
#include "Core/Client.hpp"
#include "Core/Reactor.hpp"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(const ServerConfig &config)
	: config_(config), socketFd_(socket(PF_INET, SOCK_STREAM, 0))
{
	if (socketFd_ == -1) {
		throw std::runtime_error(std::string("socket: ") + strerror(errno));
	}

	int yes = 1;
	if (setsockopt(socketFd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) ==
		-1) {
		throw std::runtime_error(std::string("setsockopt: ") + strerror(errno));
	}

	struct sockaddr_in addr = {};
	addr.sin_family			= PF_INET;
	addr.sin_port			= htons(config.port);
	addr.sin_addr.s_addr	= INADDR_ANY;

	if (bind(socketFd_, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		throw std::runtime_error(std::string("bind: ") + strerror(errno));
	}

	if (listen(socketFd_, BACKLOG) == -1) {
		throw std::runtime_error(std::string("listen: ") + strerror(errno));
	}
}

Server::~Server()
{
	close(socketFd_);
}

const ServerConfig &Server::config()
{
	return config_;
}

int Server::socketFd() const
{
	return socketFd_;
}

Client *Server::acceptClient()
{
	int fd = accept(socketFd_, NULL, NULL);
	if (fd == -1) {
		throw std::runtime_error(std::string("accept: ") + strerror(errno));
	}

	Client *client = NULL;

	try {
		client = new Client(*this, fd);
	}
	catch (...) {
		close(fd);
		throw;
	}

	return client;
}
