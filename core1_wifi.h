#pragma once

#include "myconfig.h"
#include <atomic>


extern std::atomic_bool core1_watchdog;
void core1_main();