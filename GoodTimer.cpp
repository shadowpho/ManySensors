#include "hardware/timer.h"
#include "pico/time.h"

#include "GoodTimer.h"

struct repeating_timer timer_core0[(uint8_t)TIMER_FLAGS_CORE0::NUM_OF_ELEMENTS];
struct repeating_timer timer_core1[(uint8_t)TIMER_FLAGS_CORE1::NUM_OF_ELEMENTS];

std::atomic<uint32_t> timer_flags_core0;
std::atomic<uint32_t> timer_flags_core1;

#include <stdio.h>
bool timer_flag_callback_core0(repeating_timer_t *rt)
{
    uint32_t flag = (uint32_t)rt->user_data;
    atomic_fetch_or(&timer_flags_core0, flag);
    return true; // keep repeating
}

bool timer_flag_callback_core1(repeating_timer_t *rt)
{
    uint32_t flag = (uint32_t)rt->user_data;
    atomic_fetch_or(&timer_flags_core1, flag);
    return true; // keep repeating
}

void init_timers_core0()
{
    for (int i = 0; i < (uint8_t)TIMER_FLAGS_CORE0::NUM_OF_ELEMENTS; i++)
    {
        assert(true == add_repeating_timer_ms(timer_time_core0((TIMER_FLAGS_CORE0)i), timer_flag_callback_core0, (void *)(1 << i), &timer_core0[i]));
        sleep_ms(100); // offset timers
    }
}

struct alarm_pool* core1_alarm_pool;
void init_timers_core1()
{
    
    core1_alarm_pool = alarm_pool_create_with_unused_hardware_alarm(16);
    for (int i = 0; i < (uint8_t)TIMER_FLAGS_CORE1::NUM_OF_ELEMENTS; i++)
    {
        assert(true == alarm_pool_add_repeating_timer_ms(core1_alarm_pool, timer_time_core1((TIMER_FLAGS_CORE1)i), timer_flag_callback_core1, (void *)(1 << i), &timer_core1[i]));
        sleep_ms(100); // offset timers
    }
}

void timer_change_duration_core0(TIMER_FLAGS_CORE0 tmr, uint32_t new_duration)
{
    assert(tmr<=TIMER_FLAGS_CORE0::NUM_OF_ELEMENTS);
    assert(new_duration < 4295000); //limit of 4295 seconds

    assert(cancel_repeating_timer(&timer_core0[(uint8_t) tmr]));
    atomic_fetch_and(&timer_flags_core0, !(1 << (uint32_t)tmr)); //reset the flag! very important
    assert(true == add_repeating_timer_ms(new_duration, timer_flag_callback_core0, (void *)(1 << (uint32_t)tmr), &timer_core0[(uint8_t)tmr]));
}