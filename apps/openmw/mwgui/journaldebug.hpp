#ifndef JOURNAL_DEBUG_HPP
#define JOURNAL_DEBUG_HPP

#include <android/log.h>
#include <iomanip>
#include <sstream>

// 用法: JLOG("some text " << std::hex << value << " more text")
// 输出到 Android logcat, tag = JOURNAL-DEBUG
#define JLOG(expr) \
    do { \
        std::ostringstream _jlog_oss; \
        _jlog_oss << expr; \
        __android_log_print(ANDROID_LOG_INFO, "JOURNAL-DEBUG", "%s", _jlog_oss.str().c_str()); \
    } while (0)

#endif
