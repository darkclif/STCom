#include "Random.h"

uint64_t stc::GetRandomUID64()
{
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    return dis(gen);
}