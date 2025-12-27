#include <stdio.h>
#include "pico/stdlib.h"

#include "hardware/watchdog.h"
#include "pico/cyw43_arch.h"

#include "pico/multicore.h"
#include "hardware/timer.h"
#include "pico/time.h"
#include <atomic>

#include "core1_wifi.h"
#include "i2c.h"
#include "uart.h"
#include "logging.h"
#include "hdc302x.h"




bool watchdog_loop(struct repeating_timer *t)
{
    watchdog_update();
    return true;
}

std::atomic<bool> flag_i2c = false;
std::atomic<bool> flag_uart = false;

bool time_for_i2c(struct repeating_timer *t)
{
    flag_i2c = true;
    return true;
}
bool time_for_uart(struct repeating_timer *t)
{
    flag_uart = true;
    return true;
}



int main()
{
    struct repeating_timer watchdog_timer;
    struct repeating_timer i2c_timer;
    struct repeating_timer uart_timer;
    

    stdio_init_all();

    if(false==add_repeating_timer_ms(500, watchdog_loop, NULL, &watchdog_timer))
    {
        printf("Failed to run timer for watchdog!\n"); return -2;
    }
    watchdog_enable(5000,true);
    sleep_ms(5000); //time for USB to connect


    
    if (watchdog_caused_reboot()) {
        printf("Rebooted by Watchdog!\n");

    }
    //watchdog_enable(5000, 1);

    multicore_launch_core1(core1_main);
    if(false==add_repeating_timer_ms(5000, time_for_i2c, NULL, &i2c_timer))
    {
        printf("Failed to run timer for i2c!\n"); return -3;
    }    
    if(false==add_repeating_timer_ms(10000, time_for_uart, NULL, &uart_timer))
    {
    printf("Failed to run timer for uart!\n"); return -4;
    }   

    init_i2c();

    init_uart();



    while (true) {
        if(flag_i2c==true)
        {
            flag_i2c = false;
            printf("Serviced I2C\n");
        }
        if(flag_uart==true)
        {
            flag_uart = false;
            printf("Serviced UART\n");
        }

        sleep_ms(1000);
    }
    return 0;
}
