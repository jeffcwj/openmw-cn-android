#include "include/encoding.h"
#include <iconv.h>
#include <stdexcept>

const char* charset = "GBK"; // CP1251 | GBK

char* Encoding::ToUTF8(const char* s)
{
    void* cd;
    cd = iconv_open("UTF-8", charset);
    size_t len = strlen(s);
    size_t outlen = len * 4;
    char* buf = new char[outlen + 1];
    memset(buf, 0, outlen + 1);
    char* out = buf;
    iconv(cd, (char**)&s, &len, &out, &outlen);
    iconv_close(cd);
    return buf;
}

std::string Encoding::ToUTF8(const std::string& s)
{
    iconv_t cd = iconv_open("UTF-8", charset);
    if (cd == (iconv_t) -1) {
        //spdlog::error("iconv_open failed: {}", strerror(errno));
        throw std::logic_error("iconv open failed");
        return "";
    }

    std::string in = s;
    std::string out;
    out.resize(in.size() * 4);
    char* inbuf = const_cast<char*>(in.data());
    char* outbuf = const_cast<char*>(out.data());
    size_t inbytesleft = in.size();
    size_t outbytesleft = out.size();
    size_t res = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    iconv_close(cd);
    if (res == (size_t) -1) {
        // throw std::logic_error("iconv convert failed");
        //spdlog::error("iconv failed: {}", strerror(errno));
        return "";
    }
    out.resize(out.size() - outbytesleft);
    return out;
}



std::string Encoding::FromUTF8(const std::string &s)
{
    std::string ns;
    void* cd = iconv_open(charset, "UTF-8");
    size_t len = s.size();
    size_t outlen = len * 4;
    char* buf = new char[outlen + 1];
    memset(buf, 0, outlen + 1);
    char* in = (char*)s.c_str();
    char* out = buf;
    iconv(cd, &in, &len, &out, &outlen);
    iconv_close(cd);
    ns = std::string(buf);
    return ns;
}

char* Encoding::FromUTF8(const char* s)
{
    void* cd;
    cd = iconv_open(charset, "UTF-8");
    size_t len = strlen(s);
    size_t outlen = len * 4;
    char* buf = new char[outlen + 1];
    memset(buf, 0, outlen + 1);
    char* out = buf;
    iconv(cd, (char**)&s, &len, &out, &outlen);
    iconv_close(cd);
    return buf;
}
