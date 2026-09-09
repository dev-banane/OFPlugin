#pragma once

#include <cmath>

inline double HzToMhz(long long hz)
{
    return static_cast<double>(llround(static_cast<double>(hz) / 1000.0)) / 1000.0;
}
