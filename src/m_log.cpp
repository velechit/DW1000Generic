#include "m_log.h"

#include <cstdarg>
#include <unordered_map>

namespace m_log
{
    enum class LOG_LEVEL
    {
        m_VERBOSE = 0,
        m_DEBUG = 1,
        m_INFO = 2,
        m_WARNING = 3,
        m_ERROR = 4,
    };
	
	const char *_lvl_msg_prefix[5]{
		"*V: ",
		"*D: ",
		"*I: ",
		"*W: ",
		"*E: "
	};

    static LOG_LEVEL gloablLogLevels;
    static std::unordered_map<std::string, LOG_LEVEL> logLevels;

    void setup()
    {
        gloablLogLevels = LOG_LEVEL::m_INFO;
        
        logLevels[LOG_DW1000] = LOG_LEVEL::m_VERBOSE;
        logLevels[LOG_DW1000_MSG] = LOG_LEVEL::m_VERBOSE;
    }

    void log(std::string const& tag, LOG_LEVEL level, const char* msg, std::va_list args)
    {
        LOG_LEVEL minLevel = logLevels.count(tag) ? logLevels[tag] : gloablLogLevels;
        if(level >= minLevel)
        {
			printf(_lvl_msg_prefix[(int)level]);
			vprintf(msg,args);
            va_end(args);
			printf("\n");
        }
    }

    void log_err(std::string const& tag, const char* msg, ...) { std::va_list args; va_start(args, msg); log(tag, LOG_LEVEL::m_ERROR, msg, args); }
    void log_war(std::string const& tag, const char* msg, ...) { std::va_list args; va_start(args, msg); log(tag, LOG_LEVEL::m_WARNING, msg, args); }
    void log_inf(std::string const& tag, const char* msg, ...) { std::va_list args; va_start(args, msg); log(tag, LOG_LEVEL::m_INFO, msg, args); }
    void log_dbg(std::string const& tag, const char* msg, ...) { std::va_list args; va_start(args, msg); log(tag, LOG_LEVEL::m_DEBUG, msg, args); }
    void log_vrb(std::string const& tag, const char* msg, ...) { std::va_list args; va_start(args, msg); log(tag, LOG_LEVEL::m_VERBOSE, msg, args); }
}