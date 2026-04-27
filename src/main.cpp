#include "Config/Config.hpp"
#include "Config/ConfigParser.hpp"
#include "Config/ConfigTypes.hpp"
#include "Core/Reactor.hpp"
#include "Http/Reader.hpp"
#include <cstddef>
#include <exception>
#include <ios>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <vector>

// static Config newMockConfig()
// {
// 	std::vector<ServerConfig> servers;

// 	ServerConfig s = {};
// 	s.port		   = 6969;

// 	servers.push_back(s);

// 	return Config(servers);
// }

int main(int ac, char **av)
{
	std::string config_path = "default.conf";
  	if (ac == 2)
    	config_path = av[1];

	ConfigParser parser(config_path);
    const std::vector<ServerConfig> &servers = parser.getServers();
    if (servers.empty()) {
      std::cerr << "Error: no server block found in config file" << std::endl;
      return 1;
    }




	// Config config = newMockConfig();
	// Config config = newMockConfig();
	
	Reactor reactor(servers);

	reactor.run();
}
