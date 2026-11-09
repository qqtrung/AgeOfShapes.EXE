#ifndef WIFI_PER_LOCK_HPP
#define WIFI_PER_LOCK_HPP

class WifiPerfLock
{
public:
    static void acquire();
    static void release();
};

#endif
