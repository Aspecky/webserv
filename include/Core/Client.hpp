#pragma once

class Client {
  public:
	Client(int socketFd);
	~Client();

	int socket() const;

  private:
	Client(const Client &other);
	Client &operator=(const Client &other);

	int socket_;
};
