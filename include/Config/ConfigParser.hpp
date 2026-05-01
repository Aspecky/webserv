#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "ConfigTypes.hpp"
#include <string>
#include <vector>


class ConfigParser {
    private:
        std::vector<ServerConfig> servers_;
        std::vector<std::string>  tokens_;
        size_t                    pos_;

        void    tokenize_(const std::string& content) ;

        std::string peek_();
        std::string next_();

        void expect_(const std::string& expected) ;
        bool end_() const ;

        void SkipWhiteSpaces_();

        ServerConfig parseServer_();
        LocationConfig parseLocation_();
        
    public:
        ConfigParser(const std::string& file_path);

        const std::vector<ServerConfig>& getServers() const;
        std::string trim(const std::string& str, char sep);
        void printTokens() const ;
        void printServerConfig(const ServerConfig& server);

};

#endif