#include "Core/Client.hpp"
#include "Core/Server.hpp"
#include "Http/HttpRequest.hpp"
#include <cstddef>
#include <unistd.h>

Client::Client(Server &server, int socketFd)
	: server_(server), socket_(socketFd)
{
}

Client::~Client()
{
	close(socket_);
}

int Client::socket() const
{
	return socket_;
}

// TODO: Unecessary wrapped function
void Client::onReceive(const char *buf, size_t n)
{
	request_.feed(buf, n);
}

const HttpRequest &Client::request() const
{
	return request_;
}

bool Client::hasResponse() const
{
	return !writeBuffer_.empty();
}

void Client::consumeResponse(size_t n)
{
	writeBuffer_.erase(0, n);
}

const char *Client::responseData() const
{
	return writeBuffer_.data();
}

size_t Client::responseSize() const
{
	return writeBuffer_.size();
}

bool Client::shouldClose() const
{
	return false;
}
