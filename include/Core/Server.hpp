#pragma once

class Server {
  public:
	Server(int socketFd);
	~Server();

	int socket() const;

  private:
	Server(const Server &);
	Server &operator=(const Server &);

	int socket_;
};
