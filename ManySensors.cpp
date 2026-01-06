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

#include "DataHandle.h"
#include "bsec/BSECglue.h"

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
    watchdog_enable(5000, true);
    assert(init_hdc302x() == true);
    assert(init_BMP280() == true);
    assert(init_SCD30() == true);
    assert(init_VEML7700() == true);
    BSEC_BME_init();
    init_uart();
    setup_PMS7003();
    sleep_ms(5); // wait for things to reset

    assert(start_auto_hdc302x() == true);
    assert(start_auto_BMP280() == true);
    uint16_t pm1, pm2p5, pm10;

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
            if (true == get_data_hdc302x(&temp, &rh))
                printf("HDC302x:%f, %f\n", temp, rh);
            else
                printf("HDC302x error\n");
        }
        if (flags & (uint32_t)TIMER_FLAGS_CORE0::scd30)
        {
            float temp3, rh2, co2;
            if (true == get_data_SCD30(&co2, &temp3, &rh2))
                printf("SCD30:%f, %f, %f\n", co2, temp3, rh2);
            else
                printf("SCD30 error\n");
        }
        if (flags & (uint32_t)TIMER_FLAGS_CORE0::bmp280)
        {
            float temp2, press;
            if (true == get_data_BMP280(&temp2, &press))
                printf("BMP280:%f, %f\n", temp2, press);
            else
                printf("BMP280 error\n");
        }
        if (flags & (uint32_t)TIMER_FLAGS_CORE0::veml7700)
        {
            float light;
            uint32_t ms_return;
            int veml_state_ret = process_VEML7700(&light, &ms_return);
            if (veml_state_ret == 0)
            {
                printf("VEML data: %f,%i\n", light, ms_return);
                assert(-1 == process_VEML7700(&light, &ms_return));
            }
            else if (veml_state_ret == -2)
            {
                printf("VEML error?\n");
            }
            // else veml_state==-1 and now veml_state==-1
            timer_change_duration_core0(TIMER_FLAGS_CORE0::veml7700, ms_return);
        }
        if (flags & (uint32_t)TIMER_FLAGS_CORE0::bme688)
        {
            float t4, p4, h4, VOC;
            int ret = BSEC_BME_loop(&t4, &p4, &h4, &VOC);
            if (ret != 0)
            {
                printf("BME/BSEC LOOP FAIL!!! %i\n", ret);
            }
            if (ret == 100)
            {
                BSEC_BME_init();
            }
            printf("...%f,%f,%f,%f",t4,p4,h4,VOC);
            timer_change_duration_core0(TIMER_FLAGS_CORE0::bme688, BSEC_desired_sleep_us()/1000);
        }
        if (flags & (uint32_t)TIMER_FLAGS_CORE0::SEND_TO_CORE1)
        {
            for (int i = 0; i < (uint8_t)SENSORS::NUM_OF_ELEMENTS; i++)
            {
                float d1, d2, d3, d4;
                remove_CMA(&sensor_CMA_data[i], &d1, &d2, &d3, &d4);
                printf("%s:%.1f,%.1f,%.1f,%.1f\n", SENSORS_STRING[i], d1, d2, d3, d4);
                // XXX convert to JSON, sned to site
            }
        }
        sleep_ms(100);
    }
    return 0;
}
