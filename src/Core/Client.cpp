#include "Core/Client.hpp"
#include "Core/Server.hpp"
#include "Http/HttpRequest.hpp"
#include <cstddef>
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
	if (!parser_.feed(request_, buf, n)) {
		std::string		   body = "400 Bad Request";
		std::ostringstream oss;
		oss << body.size();
		writeBuffer_ = "HTTP/1.1 400 Bad Request\r\n";
		writeBuffer_ += "Content-Type: text/plain\r\n";
		writeBuffer_ += "Content-Length: " + oss.str() + "\r\n";
		writeBuffer_ += "Connection: close\r\n";
		writeBuffer_ += "\r\n";
		writeBuffer_ += body;

		shouldClose_ = true;
		return;
	}

	if (parser_.isComplete()) {
		std::string echo;
		echo += request_.method() + " " + request_.uri() + " " +
				request_.version() + "\r\n";
		const std::map<std::string, std::string> &hdrs = request_.headers();
		for (std::map<std::string, std::string>::const_iterator it =
				 hdrs.begin();
			 it != hdrs.end(); ++it) {
			echo += it->first + ": " + it->second + "\r\n";
		}
		echo += "\r\n";
		echo += request_.body();

		writeBuffer_ = "HTTP/1.1 200 OK\r\n";
		writeBuffer_ += "Content-Type: text/plain\r\n";
		std::ostringstream oss;
		oss << echo.size();
		writeBuffer_ += "Content-Length: " + oss.str() + "\r\n";
		writeBuffer_ += "\r\n";
		writeBuffer_ += echo;

		shouldClose_ = false;
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
