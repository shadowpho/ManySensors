#pragma once
#include <atomic>

#include <cstdint>

extern std::atomic<uint32_t> timer_flags_core0;
extern std::atomic<uint32_t> timer_flags_core1;

enum class TIMER_FLAGS_CORE0 : uint8_t
{
    watchdog, // 0
    pms7003,
    hdc_302x,
    scd30,
    bmp280,
    veml7700,
    bme688,
    NUM_OF_ELEMENTS
};

enum class TIMER_FLAGS_CORE1 : uint8_t
{
    wifi,
    stats,
    NUM_OF_ELEMENTS
};

constexpr uint32_t timer_time_core0(TIMER_FLAGS_CORE0 f)
{
    switch (f)
    {
    case TIMER_FLAGS_CORE0::watchdog:
        return 500;
    case TIMER_FLAGS_CORE0::pms7003:
        return 200;
    case TIMER_FLAGS_CORE0::hdc_302x:
        return 1500;
    case TIMER_FLAGS_CORE0::scd30:
        return 3000;
    case TIMER_FLAGS_CORE0::bmp280:
        return 1500;
    case TIMER_FLAGS_CORE0::veml7700:
        return 1000;
    case TIMER_FLAGS_CORE0::bme688:
        return 1000;
    default:
        return 0;
    }
}
constexpr uint32_t timer_time_core1(TIMER_FLAGS_CORE1 f)
{
    switch (f)
    {
    case TIMER_FLAGS_CORE1::wifi:
        return 100000;
    case TIMER_FLAGS_CORE1::stats:
        return 300000;
    default:
        return 0;
    }
}

void init_timers_core0();
void init_timers_core1();