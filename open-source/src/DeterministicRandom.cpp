#include "DeterministicRandom.hpp"

DeterministicRandom::DeterministicRandom(uint32_t seed) : mState(seed)
{
}

void DeterministicRandom::setSeed(uint32_t seed)
{
    mState = seed;
}

int DeterministicRandom::rand()
{
    mState = (mState * 1103515245 + 12345) & 0x7fffffff;
    return static_cast<int>(mState);
}

int DeterministicRandom::randInt(int max)
{
    return rand() % max;
}

int DeterministicRandom::randIntRange(int min, int max)
{
    return min + rand() % (max - min + 1);
}

float DeterministicRandom::randFloat()
{
    return static_cast<float>(rand()) / static_cast<float>(0x7fffffff);
}

float DeterministicRandom::randFloatRange(float min, float max)
{
    return min + randFloat() * (max - min);
}
