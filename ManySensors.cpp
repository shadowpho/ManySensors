#include <stdio.h>
#include "pico/stdlib.h"

#include "hardware/watchdog.h"
#include "pico/cyw43_arch.h"

#include "pico/multicore.h"

#include <atomic>

#include "GoodTimer.h"
#include "core1_wifi.h"
#include "i2c.h"
#include "uart.h"
#include "logging.h"

#include "hdc302x.h"
#include "PMS7003.h"
#include "veml7700.h"
#include "bmp280.h"
#include "scd30.h"

int main()
{
    watchdog_disable();
    stdio_init_all();
    init_timers_core0();
       
    sleep_ms(5000); // time for USB to connect

    if (watchdog_caused_reboot())
    {
        printf("Rebooted by Watchdog!\n");
    }
    multicore_launch_core1(core1_main);
   
    init_i2c();
    assert(init_hdc302x()==true);
    init_uart();
    setup_PMS7003();

    sleep_ms(5); //wait for things to reset
    assert(start_auto_hdc302x()==true);
    assert(start_auto_BMP280()==true);

    uint16_t pm1, pm2p5, pm10;
    watchdog_enable(5000, true);
    while (true)
    {
        uint32_t flags = std::atomic_exchange(&timer_flags_core0, 0);
        if (flags & (uint32_t)TIMER_FLAGS_CORE0::watchdog)
        {
            watchdog_update();
        }
        if (flags & (uint32_t)TIMER_FLAGS_CORE0::pms7003)
        {
            if (true == read_from_PMS(&pm1, &pm2p5, &pm10))
                printf("Got data! %i, %i, %i\n", pm1, pm2p5, pm10);
        }
        if (flags & (uint32_t)TIMER_FLAGS_CORE0::hdc_302x)
        {
            float temp, rh;
            if (true== get_data_hdc302x(&temp, &rh))
                printf("HDC302x:%f, %f\n", temp, rh);
            else
                printf("HDC302x error\n");
        }
        if (flags & (uint32_t)TIMER_FLAGS_CORE0::scd30)
        {
             printf("Serviced I2C\n");
        }
        if (flags & (uint32_t)TIMER_FLAGS_CORE0::bmp280)
        {
             float temp2, press;
            if (true== get_data_BMP280(&temp2, &press))
                printf("BMP280:%f, %f\n", temp2, press);
            else
                printf("BMP280 error\n");
        }
        if (flags & (uint32_t)TIMER_FLAGS_CORE0::veml7700)
        {
             printf("Serviced I2C\n");
        }
        if (flags & (uint32_t)TIMER_FLAGS_CORE0::bme688)
        {
             printf("Serviced I2C\n");
        }
        sleep_ms(100);
    }
    return 0;
}
