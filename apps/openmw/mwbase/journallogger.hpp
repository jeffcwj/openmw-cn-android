#ifndef OPENMW_MWBASE_JOURNALLOGGER_HPP
#define OPENMW_MWBASE_JOURNALLOGGER_HPP

#include <spdlog/spdlog.h>
#include <spdlog/sinks/android_sink.h>
#include <memory>
#include <string>

namespace MWBase
{
    class JournalLogger
    {
    public:
        static void init()
        {
            if (!sLogger)
            {
                // Create Android logger
                std::string tag = "OpenMW-Journal";
                auto android_sink = std::make_shared<spdlog::sinks::android_sink_mt>(tag);
                sLogger = std::make_shared<spdlog::logger>("journal", android_sink);
                
                // Set log level to trace to see all messages
                sLogger->set_level(spdlog::level::trace);
                sLogger->flush_on(spdlog::level::info);
                
                // Register it as default logger
                spdlog::set_default_logger(sLogger);
                
                spdlog::info("JournalLogger initialized");
            }
        }
        
        static std::shared_ptr<spdlog::logger> get()
        {
            if (!sLogger)
                init();
            return sLogger;
        }
        
    private:
        static std::shared_ptr<spdlog::logger> sLogger;
    };
}

#endif
