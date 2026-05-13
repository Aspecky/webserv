#include "Core/Client.hpp"
#include "Core/Server.hpp"
#include "Http/HttpResponse.hpp"
#include "Http/RequestHandler.hpp"
#include <cstddef>
#include <iostream>
#include <string>
#include <unistd.h>

Client::Client(Server &server, int socketFd)
	: server_(server), socket_(socketFd),
	  parser_(server_.config().max_body_size)
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
	HttpResponse   res;
	RequestHandler handler(server_.config());

	if (parser_.feed(buf, n, request_)) {
		if (parser_.isComplete()) {
			handler.handle(request_, res);
			res.serialize(writeBuffer_);
		}
	}
	else {
		handler.handleError(parser_.statusCode(), res);
	}
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
