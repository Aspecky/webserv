#pragma once

#include "Config/ServerConfig.hpp"
#include <vector>

class Config {
  public:
	Config(const std::vector<ServerConfig> &servers);
	// Config(const Config &other);
	~Config();

	// Config &operator=(const Config &other);

	const std::vector<ServerConfig> &servers() const;

  private:
	std::vector<ServerConfig> servers_;
};
