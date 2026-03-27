#include "Config/Config.hpp"
#include "Config/ServerConfig.hpp"
#include "Core/Reactor.hpp"
#include <cstddef>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <vector>

static Config newMockConfig()
{
	std::vector<ServerConfig> servers;

	ServerConfig s = {};
	s.port		   = 8080;

	servers.push_back(s);

	return Config(servers);
}

int main()
{
	Config config = newMockConfig();

	Reactor reactor(config);

	reactor.run();
}
