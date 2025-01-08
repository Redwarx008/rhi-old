#pragma once

#include "rhi/rhi_struct.h"

namespace rhi
{

	extern DebugMessageCallbackFunc g_DebugMessageCallback;

	template<typename... ArgsType>
	void logMsg(MessageSeverity severity, const char* functionName, int line, const ArgsType&... args)
	{
		std::string prefix;
		if (severity == MessageSeverity::Error)
		{
			prefix = "Error";
#if defined(_WIN32)
			prefix = "\033[31m" + prefix + "\033[0m";
#endif
		}
		else if (severity == MessageSeverity::Warning)
		{
			prefix = "Warning";
#if defined(_WIN32)
			prefix = "\033[33m" + prefix + "\033[0m";
#endif
		}
		std::stringstream ss;
		auto msg = combineString(args...);
		ss << prefix << " in " << functionName << " at line " << line << " : " << msg << '\n';
		if (g_DebugMessageCallback != nullptr)
		{
			g_DebugMessageCallback(severity, ss.str().c_str());
		}
		else
		{
			std::cerr << ss.str();
		}
		if (severity >= MessageSeverity::Error)
		{
			throw std::runtime_error(std::move(ss));
		}
	}

#define LOG_ERROR(...)																			 \
	do																							 \
	{																						     \
         logMsg(MessageSeverity::Error, __FUNCTION__, __LINE__, ##__VA_ARGS__);                \
	} while (false)																				 \



#define LOG_WARNING(...)																			 \
	do																							 \
	{																						     \
         logMsg(MessageSeverity::Warning, __FUNCTION__, __LINE__, ##__VA_ARGS__);                \
	} while (false)																				 \




#ifdef NDEBUG
#define ASSERT_MSG(EXPR, ...) ((void)0)
#else
#define ASSERT_MSG(EXPR, ...)   \
			do					\
			{					\
				if(!(EXPR))		\
				{				\
					logMsg(MessageSeverity::Error, __FUNCTION__, __LINE__, __VA_ARGS__);	\
                    abort();\
				}	        \
			} while (false)		
#endif // NDEBUG


}