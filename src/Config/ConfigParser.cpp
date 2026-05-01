#include "Config/ConfigParser.hpp"


#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <stdlib.h>




ConfigParser::ConfigParser(const std::string& file_path){

    std::ifstream config(file_path.c_str());
    if(!config.is_open())
        throw std::runtime_error("cannot open config file : " + file_path);

    std::ostringstream oss;
    oss << config.rdbuf();

    std::string content = oss.str();

    tokenize_(content);

    pos_ = 0;
    while (!end_()) {
        std::string tok = next_();
        if (tok == "server")
            servers_.push_back(parseServer_());
        else
            throw std::runtime_error("Unexpected token: " + tok);
    }
}

// ─── Tokenizer ─────────────────────────────────────────────────

void ConfigParser::tokenize_(const std::string& content){

    std::string token;

    for (size_t i = 0; i < content.size(); i++)
    {
        char c = content[i];

        // skip comments
        if (c == '#')
        {
            while (i < content.size() && content[i] != '\n')
                i++;
            continue;
        }

        // braces and semicolons are standalone tokens
        if (c == '{' || c == '}' || c == ';')
        {
            if (!token.empty())
            {
                tokens_.push_back(token);
                token.clear();
            }
            tokens_.push_back(std::string(1, c));
            continue;
        }

        // whitespace = token separator
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            if (!token.empty())
            {
                tokens_.push_back(token);
                token.clear();
            }
            continue;
        }

        token += c;
    }

    if (!token.empty())
        tokens_.push_back(token);

}



// ─── Helpers ─────────────────────────────────────────────────

void ConfigParser::expect_(const std::string& expected){

    if (end_() || tokens_[pos_] != expected)
    {
        throw std::runtime_error("Expected '" + expected + "' but found " + (end_() ? "end of file" : tokens_[pos_]));
    }
    pos_++;
}

std::string ConfigParser::next_(){
    if (end_())
        throw std::runtime_error("Unexpected end of file");
    return tokens_[pos_++];
}

std::string ConfigParser::peek_() {
    if (end_())
        throw std::runtime_error("Unexpected end of file");
    return tokens_[pos_];
}

bool ConfigParser::end_() const {
    return pos_ >= tokens_.size();
}



void ConfigParser::printTokens() const
{
    std::cout << "Tokens (" << tokens_.size() << "):\n";
    
    for (size_t i = 0; i < tokens_.size(); i++)
    {
        std::cout << "[" << i << "] = \"" << tokens_[i] << "\"\n";
    }
}

// ─── Server Block ─────────────────────────────────────────────────

ServerConfig ConfigParser::parseServer_(){
   
    ServerConfig config;
    expect_("{");

    while(peek_() != "}")
    {
        std::string key = next_();
        if(key == "listen"){
            config.port = atoi(next_().c_str());
            expect_(";");
        }
        else if(key == "host"){
            std::string ter = next_();
            ter = trim(ter, '"');
            config.host = ter;
            // config.host = next_();
            expect_(";");
        }
        else if(key == "max_body_size"){
            config.max_body_size = atoi(next_().c_str());
            expect_(";");
        }
        else if(key == "error_page"){
            int code = atoi(next_().c_str());
            config.error_pages[code] = next_();
            expect_(";");
        }
        else if(key == "location"){
            std::string path = next_();
            LocationConfig locConfig = parseLocation_();
            locConfig.path = path;
            config.locations[path] = locConfig;
        }
        else{
            throw std::runtime_error("Unknown server directive: " + key);
        }

    }
    expect_("}");
    return config;
}


// ─── Location Block ─────────────────────────────────────────────────

LocationConfig ConfigParser::parseLocation_(){
    LocationConfig loc;
    expect_("{");

    while(peek_() != "}"){

        std::string key = next_();
        if (key == "root"){
            loc.root = next_();
            expect_(";");
        }
        else if(key == "index"){
            loc.index = next_();
            expect_(";");
        }
        else if(key == "methods"){
            while(peek_() != ";")
                loc.methods.push_back(next_());
            expect_(";");
        }
        else if(key == "directory_listing"){
            loc.directory_listing = (next_() == "on");
            expect_(";");
        }
        else if (key == "upload_store"){
            loc.upload_store = next_();
            expect_(";");
        }
        else if (key == "redirect"){
            loc.redirect = next_();
            expect_(";");
        }
        else if (key == "cgi"){
            std::string ext = next_();
            std::string handler = next_();
            loc.cgi[ext] = handler;
            expect_(";");
        }
        else{
            throw std::runtime_error("Unknown location directive: " + key);
        }

    }
    expect_("}");
    return loc;
}


// ______ Getters ___________________________________________________

const std::vector<ServerConfig>& ConfigParser::getServers() const
{
    return servers_; 
}


std::string ConfigParser::trim(const std::string& str, char sep) {
    size_t first = str.find_first_not_of(sep);
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(sep);
    return str.substr(first, (last - first + 1));
}



void printServerConfig(const ServerConfig& server)
{
    std::cout << "server {\n";
    std::cout << "    listen " << server.port << ";\n";
    std::cout << "    host \"" << server.host << "\";\n";
    std::cout << "    max_body_size " << server.max_body_size << ";\n";

    for (std::map<int, std::string>::const_iterator it = server.error_pages.begin();
         it != server.error_pages.end(); ++it) {
        std::cout << "    error_page " << it->first << " " << it->second << ";\n";
    }

    for (std::map<std::string, LocationConfig>::const_iterator it = server.locations.begin();
         it != server.locations.end(); ++it) {
        const LocationConfig& loc = it->second;
        std::cout << "\n    location " << it->first << " {\n";

        if (!loc.root.empty())
            std::cout << "        root " << loc.root << ";\n";
        if (!loc.index.empty())
            std::cout << "        index " << loc.index << ";\n";

        if (!loc.methods.empty()) {
            std::cout << "        methods";
            for (size_t i = 0; i < loc.methods.size(); ++i)
                std::cout << " " << loc.methods[i];
            std::cout << ";\n";
        }

        std::cout << "        directory_listing " << (loc.directory_listing ? "on" : "off") << ";\n";

        if (!loc.upload_store.empty())
            std::cout << "        upload_store " << loc.upload_store << ";\n";

        for (std::map<std::string, std::string>::const_iterator c = loc.cgi.begin();
             c != loc.cgi.end(); ++c) {
            std::cout << "        cgi " << c->first << " " << c->second << ";\n";
        }

        if (!loc.redirect.empty())
            std::cout << "        return " << loc.redirect << ";\n";

        std::cout << "    }\n";
    }

    std::cout << "}\n";
}
