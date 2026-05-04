#ifndef HELPER_HPP
#define HELPER_HPP




#include <string>
#include <vector>


namespace RequestHelpers {

    extern const std::string Empty;

    std::string getContentType(const std::string& ext);

    std::string sizeToString(std::size_t);
    std::string toLower(const std::string &s);
    std::string extractBoundary(const std::string &contentType);
    std::string extractFilename(const std::string &disposition);
    std::string getDisposition(const std::string &partHeaders);
}

#endif