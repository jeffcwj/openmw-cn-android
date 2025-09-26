#pragma once

#include <string>

class Encoding {
public:
    static std::string ToUTF8(const std::string& s);

    static std::string FromUTF8(const std::string &s);

    static char *FromUTF8(const char *s);

    static char *ToUTF8(const char *s);
};
