#include "Core/Client.hpp"

Client::Client(int socketFd) : socket_(socketFd)
{
}

int Client::socket() const
{
	return socket_;
}
