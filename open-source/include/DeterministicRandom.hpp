#ifndef DETERMINISTIC_RANDOM_HPP
#define DETERMINISTIC_RANDOM_HPP

#include <cstdint>

struct DeterministicRandom
{
    explicit DeterministicRandom(uint32_t seed = 1);

    void  setSeed(uint32_t seed);
    int   rand();
    int   randInt(int max);
    int   randIntRange(int min, int max);
    float randFloat();
    float randFloatRange(float min, float max);

    uint32_t mState;
};

#endif
