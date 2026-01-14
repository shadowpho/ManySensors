#pragma once
#include <atomic>

#include <cstdint>

extern std::atomic<uint32_t> timer_flags_core0;
extern std::atomic<uint32_t> timer_flags_core1;

#define is_flag_set(flag_variable, flag) ((flag_variable & (1<<(uint32_t)flag))==(1<<(uint32_t)flag))

enum class TIMER_FLAGS_CORE0 : uint8_t
{
    watchdog, // 0
    pms7003,
    hdc_302x,
    scd30,
    bmp280,
    veml7700,
    bme688,
    SEND_TO_CORE1,
    NUM_OF_ELEMENTS
};

enum class TIMER_FLAGS_CORE1 : uint8_t
{
    wifi,
    stats,
    core1_watchdog,
    NUM_OF_ELEMENTS
};

void timer_change_duration_core0(TIMER_FLAGS_CORE0 tmr, uint32_t new_duration);

constexpr uint32_t timer_time_core0(TIMER_FLAGS_CORE0 f)
{
    switch (f)
    {
    case TIMER_FLAGS_CORE0::watchdog:
        return 500;
    case TIMER_FLAGS_CORE0::pms7003:
        return 200;
    case TIMER_FLAGS_CORE0::hdc_302x:
        return 2200;
    case TIMER_FLAGS_CORE0::scd30:
        return 3000;
    case TIMER_FLAGS_CORE0::bmp280:
        return 1500;
    case TIMER_FLAGS_CORE0::veml7700:
        return 1000;
    case TIMER_FLAGS_CORE0::bme688:
        return 1000;
    case TIMER_FLAGS_CORE0::SEND_TO_CORE1:
        return 10*60*1000;
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
    case TIMER_FLAGS_CORE1::core1_watchdog:
        return 1000;
    default:
        return 0;
    }
}

void init_timers_core0();
void init_timers_core1();