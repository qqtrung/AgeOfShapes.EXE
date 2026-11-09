#include "Debug.hpp"

#ifdef __ANDROID__
#include <android/log.h>
#else
#include <iostream>
#endif

void logout(const std::string& message, bool isError)
{
#ifdef __ANDROID__
    android_LogPriority priority = isError ? ANDROID_LOG_ERROR : ANDROID_LOG_INFO;
    __android_log_print(priority, "std::cout", "%s", message.c_str());
#else
    if (isError)
    {
        std::cerr << message << std::endl;
    }
    else
    {
        std::cout << message << std::endl;
    }
#endif
}

void logout(const char* message, bool isError)
{
    logout(std::string(message), isError);
}

LogStream logout(bool isError)
{
    return LogStream(isError);
}

LogStream::~LogStream()
{
    std::string s = oss.str();
    if (!s.empty())
    {
        logout(s, isError);
    }
}

void logOut(const std::string& message, bool isError)
{
    logout(message, isError);
}

void logOut(const char* message, bool isError)
{
    logout(std::string(message), isError);
}

LogStream logOut(bool isError)
{
    return logout(isError);
}
