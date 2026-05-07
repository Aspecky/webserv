#include "Config/ConfigParser.hpp"
#include "Config/ConfigTypes.hpp"
#include "Core/Reactor.hpp"
#include "Response/Colors.hpp"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <vector>

int main(int ac, char **av)
{
	signal(SIGPIPE, SIG_IGN);

	if (ac < 2) {
		std::cout << RED << "Error: \"miss config file\"" << RESET << std::endl;
		return (EXIT_FAILURE);
	}
	std::string configPath = av[1];

	ConfigParser					 parser(configPath);
	const std::vector<ServerConfig> &servers = parser.getServers();

	// TODO: Use try catch block to report errors
	if (servers.empty()) {
		std::cerr << "Error: no server block found in config file" << std::endl;
		return 1;
	}

	Reactor reactor(servers);

	reactor.run();
}
