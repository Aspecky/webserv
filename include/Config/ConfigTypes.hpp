#ifndef CONFIG_TYPES_HPP
#define CONFIG_TYPES_HPP

#include <string>
#include <vector>
#include <map>


struct LocationConfig {
    std::string path;
    std::string root;
    std::string index;
    std::vector<std::string> methods;
    bool directory_listing;
    std::string upload_store;
    std::map<std::string, std::string> cgi;
    std::string redirect;

    LocationConfig() : directory_listing(false) {}
};


struct ServerConfig {
    std::string  host;
    int          port;
    size_t       max_body_size;
    std::map<int , std::string> error_pages;
    std::map<std::string, LocationConfig> locations;
    
    ServerConfig() : host("0.0.0.0"), port(8080), max_body_size(100000) {}
};

#endif