#include "Profiler.hpp"

#include <algorithm>
#include <iostream>

#if defined(ANDROID) || defined(__ANDROID__)
#include <SFML/System/NativeActivity.hpp>

#include <android/native_activity.h>

#include <filesystem>
#endif

std::string Profiler::getProfilePath()
{
#if defined(ANDROID) || defined(__ANDROID__)
    ANativeActivity* activity = sf::getNativeActivity();
    if (activity)
    {
        std::string dir;
        if (activity->externalDataPath && activity->externalDataPath[0] != '\0')
        {
            dir = activity->externalDataPath;
        }
        else if (activity->internalDataPath && activity->internalDataPath[0] != '\0')
        {
            dir = activity->internalDataPath;
        }

        if (!dir.empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            return dir + "/results.json";
        }
    }
    return "/sdcard/results.json";
#else
    return "results.json";
#endif
}

Profiler::Profiler()
{
    mOutputFile = getProfilePath();
    mOutputStream.open(mOutputFile);
    writeHeader();
}

void Profiler::writeHeader()
{
    if (mOutputStream.is_open())
    {
        mOutputStream << R"({"otherData": {},"traceEvents":[)";
    }
}

void Profiler::writeFooter()
{
    if (mOutputStream.is_open())
    {
        mOutputStream << "\n]}";
        mOutputStream.flush();
        mOutputStream.close();
    }
}

void Profiler::endSession()
{
    const std::scoped_lock lock(mLock);
    writeFooter();
}

Profiler& Profiler::instance()
{
    static Profiler inst;
    return inst;
}

Profiler::~Profiler()
{
    endSession();
}

void Profiler::writeProfile(const ProfileResult& result)
{
    const std::scoped_lock lock(mLock);
    if (!mOutputStream.is_open())
    {
        return;
    }

    if (mProfileCount++ > 0)
    {
        mOutputStream << ",";
    }

    std::string name;
    name.reserve(result.name.size() + 8);
    for (char c : result.name)
    {
        switch (c)
        {
            case '"':
                name += '\'';
                break;
            case '\\':
                name += "\\\\";
                break;
            case '\n':
                name += "\\n";
                break;
            case '\r':
                break;
            case '\t':
                name += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20)
                {
                    name += ' ';
                }
                else
                {
                    name += c;
                }
                break;
        }
    }

    mOutputStream << "\n{";
    mOutputStream << R"("cat":"function",)";
    mOutputStream << "\"dur\":" << (result.end - result.start) << ',';
    mOutputStream << R"("name":")" << name << "\",";
    mOutputStream << R"("ph":"X",)";
    mOutputStream << "\"pid\":0,";
    mOutputStream << "\"tid\":" << result.threadID << ",";
    mOutputStream << "\"ts\":" << result.start;
    mOutputStream << "}";

    if ((mProfileCount & 63) == 0)
    {
        mOutputStream.flush();
    }
}

ProfileTimer::ProfileTimer(const std::string& name) : m_result({.name = name, .start = 0, .end = 0, .threadID = 0})
{
    start();
}

ProfileTimer::~ProfileTimer()
{
    stop();
}

void ProfileTimer::start()
{
    static long long lastStartTime = 0;

    m_startTimepoint = std::chrono::high_resolution_clock::now();
    m_result.start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimepoint).time_since_epoch().count();

    m_result.start += (m_result.start == lastStartTime) ? 1 : 0;

    lastStartTime = m_result.start;
    m_stopped     = false;
}

void ProfileTimer::stop()
{
    if (m_stopped)
    {
        return;
    }
    auto endTimepoint = std::chrono::high_resolution_clock::now();
    m_result.end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();
    m_result.threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());
    Profiler::instance().writeProfile(m_result);
    m_stopped = true;
}
