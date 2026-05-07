#include "Core/Client.hpp"
#include "Core/Server.hpp"
#include "Http/HttpRequest.hpp"
#include "Http/RequestHandler.hpp"
#include "Http/HttpResponse.hpp"
#include <cstddef>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unistd.h>

Client::Client(Server &server, int socketFd)
	: server_(server), socket_(socketFd), shouldClose_(false)
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
	HttpResponse	   res;
	RequestHandler	   handler(server_.config());

	if (parser_.feed(buf, n, request_)) {
		if (parser_.isComplete()) {
			std::cout << "Request parser is complete\n";
			handler.handle(request_, res);
			res.serialize(writeBuffer_);

			// response_.buildResponse(request, server_.config());
		}
	}
	else {
		handler.handleError(400, res);
	}

	// parser_.feed(buf, n);
}

// const HttpRequest &Client::request() const
// {
// 	return parser_.request();
// }

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
	if (writeBuffer_.empty() && !shouldClose_) {
		parser_.reset();
		request_.reset();
	}
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
	return shouldClose_;
}
