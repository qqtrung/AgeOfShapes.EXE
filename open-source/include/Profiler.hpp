#ifndef PROFILER_HPP
#define PROFILER_HPP

#if defined(ANDROID) || defined(__ANDROID__)
#include <algorithm>
#include <filesystem>
#endif

#include <chrono>
#include <fstream>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#if defined(ANDROID) || defined(__ANDROID__)
#include <SFML/System/NativeActivity.hpp>

#include <android/native_activity.h>
#endif

#define PROFILING 0

#if PROFILING
#if defined(__GNUC__) || defined(__clang__)
#define PROFILE_FUNC_SIG __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define PROFILE_FUNC_SIG __FUNCSIG__
#else
#define PROFILE_FUNC_SIG __FUNCTION__
#endif

#define PROFILE_SCOPE(name)   ProfileTimer timer##__LINE__(name)
#define PROFILE_FUNCTION()    PROFILE_SCOPE(PROFILE_FUNC_SIG)
#define PROFILE_END_SESSION() Profiler::instance().endSession()
#else
#define PROFILE_SCOPE(name)
#define PROFILE_FUNCTION()
#define PROFILE_END_SESSION()
#endif

struct ProfileResult
{
    std::string name     = "Default";
    long long   start    = 0;
    long long   end      = 0;
    size_t      threadID = 0;
};

struct Profiler
{
    std::string   mOutputFile;
    size_t        mProfileCount = 0;
    std::ofstream mOutputStream;
    std::mutex    mLock;

    static std::string getProfilePath();

    Profiler();
    ~Profiler();

    void             writeHeader();
    void             writeFooter();
    void             endSession();
    static Profiler& instance();

    void writeProfile(const ProfileResult& result);
};

class ProfileTimer
{
    using ClockType = std::chrono::time_point<std::chrono::high_resolution_clock>;
    ClockType     m_startTimepoint;
    ProfileResult m_result;
    bool          m_stopped = false;

public:
    ProfileTimer(const std::string& name);
    ~ProfileTimer();

    void start();
    void stop();
};

#endif
