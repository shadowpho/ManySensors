#include "hardware/timer.h"
#include "pico/time.h"

#include "GoodTimer.h"

struct repeating_timer timer_core0[(uint8_t)TIMER_FLAGS_CORE0::NUM_OF_ELEMENTS];
struct repeating_timer timer_core1[(uint8_t)TIMER_FLAGS_CORE1::NUM_OF_ELEMENTS];



std::atomic<uint32_t> timer_flags_core0;
std::atomic<uint32_t> timer_flags_core1;


bool timer_flag_callback_core0(repeating_timer_t *rt) {
    uint32_t flag = (uint32_t)rt->user_data;
    atomic_fetch_or(&timer_flags_core0, flag);
    return true; // keep repeating
}

bool timer_flag_callback_core1(repeating_timer_t *rt) {
    uint32_t flag = (uint32_t)rt->user_data;
    atomic_fetch_or(&timer_flags_core1, flag);
    return true; // keep repeating
}

void init_timers_core0()
{
    for(int i=0;i<(uint8_t)TIMER_FLAGS_CORE0::NUM_OF_ELEMENTS; i++)
    {
        assert( true == add_repeating_timer_ms(timer_time_core0(i),timer_flag_callback,(void*) 1<<i, timer[i]));
        sleep_ms(100); //offset timers
    }
}

void init_timers_core1()
{
    for(int i=0;i<(uint8_t)TIMER_FLAGS_CORE1::NUM_OF_ELEMENTS; i++)
    {
        assert( true == add_repeating_timer_ms(timer_time_core1(i),timer_flag_callback,(void*) 1<<i, timer[i]));
        sleep_ms(100); //offset timers
    }
}