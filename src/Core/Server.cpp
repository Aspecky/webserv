#include "Core/Server.hpp"

Server::Server(int socketFd) : socket_(socketFd)
{
}

int Server::socket() const
{
	return socket_;
}
