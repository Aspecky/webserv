#include "Config/Config.hpp"
#include "Config/ConfigParser.hpp"
#include "Config/ConfigTypes.hpp"
#include "Core/Reactor.hpp"
#include "Http/Reader.hpp"
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <ios>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "Response/Colors.hpp"
#include "Response/Helper.hpp"

// static Config newMockConfig()
// {
// 	std::vector<ServerConfig> servers;

// 	ServerConfig s = {};
// 	s.port		   = 6969;

// 	servers.push_back(s);

// 	return Config(servers);
// }
#include <csignal>

int main(int ac, char **av)
{
	signal(SIGPIPE, SIG_IGN);

  	if (ac < 2){
		std::cout << RED << "Error: \"miss config file\"" << RESET << std::endl;
		return (EXIT_FAILURE);
	}
    std::string config_path = av[1];

	ConfigParser parser(config_path);
    const std::vector<ServerConfig> &servers = parser.getServers();
    if (servers.empty()) {
      std::cerr << "Error: no server block found in config file" << std::endl;
      return 1;
    }

	
	Reactor reactor(servers);

	reactor.run();
}
