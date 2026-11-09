#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <typeinfo>

#ifdef __ANDROID__
#include <android/log.h>
#define DEBUG_BREAK() __builtin_trap()
#elif defined(_WIN32)
#define DEBUG_BREAK() __debugbreak()
#else
#include <signal.h>
#define DEBUG_BREAK() raise(SIGTRAP)
#endif

struct LogStream
{
    std::ostringstream oss;
    bool               isError = false;

    explicit LogStream(bool error = false) : isError(error)
    {
    }

    template <typename T>
    LogStream& operator<<(const T& value)
    {
        oss << value;
        return *this;
    }

    LogStream& operator<<(std::ostream& (*)(std::ostream&))
    {
        return *this;
    }

    ~LogStream();
};

void      logout(const std::string& message, bool isError = false);
void      logout(const char* message, bool isError = false);
LogStream logout(bool isError = false);

void      logOut(const std::string& message, bool isError = false);
void      logOut(const char* message, bool isError = false);
LogStream logOut(bool isError = false);

template <typename T>
void assertImpl(bool cond)
{
    if (!cond)
    {
        logout("ASSERT FAILED", true);
        logout(typeid(T).name(), true);
        DEBUG_BREAK();
        exit(1);
    }
}

#define ASSERT(cond)   assertImpl<void>(cond)
#define EPSILON        0.000001f
#define KB(x)          ((uint64_t)1024 * (x))
#define MB(x)          ((uint64_t)1024 * KB(x))
#define GB(x)          ((uint64_t)1024 * MB(x))
#define BIT(x)         ((uint64_t)1 << (x))
#define ArraySize(arr) (sizeof((arr)) / sizeof((arr)[0]))
#define N_BIT_SIZE_T   (8 * sizeof(size_t))
#define DEBUG          0

static_assert((N_BIT_SIZE_T & (N_BIT_SIZE_T - 1)) == 0);

#endif
