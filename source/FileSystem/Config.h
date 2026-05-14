#pragma once

#include <cstdint>

class Config
{
private:
    Config() = delete;
    ~Config() = delete;

public:
    static inline constexpr int DEFAULT_FRAMERATE = 100;
    static inline constexpr int MINIMUM_FRAMERATE = 20;
    static inline constexpr int MAXIMUM_FRAMERATE = 100;

    static bool IsValidFramerateLimit(int value)
    {
        return value >= MINIMUM_FRAMERATE && value <= MAXIMUM_FRAMERATE;
    }

    static int FramerateToUSecInterval(int value)
    {
        constexpr double ONE_SECOND_IN_USEC = 1000000.0;
        double uSecInterval = ONE_SECOND_IN_USEC / (double)value;

        return (int)std::round(uSecInterval);
    }
};
