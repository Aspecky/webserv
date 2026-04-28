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

void Client::onReceive(const char *buf, size_t n)
{
	parser_.feed(request_, buf, n);

	if (parser_.isComplete()) {
		// TODO: Build response
	}
}

const HttpRequest &Client::request() const
{
	return request_;
}

bool Client::requestComplete() const
{
	return parser_.isComplete();
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
