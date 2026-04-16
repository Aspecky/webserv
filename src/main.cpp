#include "Config/ConfigParser.hpp"
#include "Core/Reactor.hpp"
#include <cstddef>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <vector>

// static Config newMockConfig()
// {
// 	std::vector<ServerConfig> servers;

// 	ServerConfig s = {};
// 	s.port		   = 8080;

// 	servers.push_back(s);

// 	return Config(servers);
// }

int main(int ac, char **av)
{
	std::string configPath = "default.conf";
	if(ac == 2)
		configPath = av[1];

	try{
		ConfigParser parser(configPath);
		const std::vector<ServerConfig> &servers = parser.getServers();
		Reactor reactor(servers);
		reactor.run();
	}
	catch(const std::exception& e){
		std::cerr << e.what() << std::endl;
		return 1;
	}
	
	// Config config = newMockConfig();
	

	// reactor.run();
}
