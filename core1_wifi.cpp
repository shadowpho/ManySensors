#include "core1_wifi.h"

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/watchdog.h"
#include "pico/cyw43_arch.h"
#include "hardware/uart.h"
#include "pico/multicore.h"
#include "hardware/timer.h"
#include "pico/time.h"
#include <atomic>

std::atomic<bool> flag_wifi = false;

bool time_for_wifi(struct repeating_timer *t)
{
    flag_wifi = true;
    return true;
}

void core1_main()
{
    struct repeating_timer wifi_timer;

    if(false==add_repeating_timer_ms(100000, time_for_wifi, NULL, &wifi_timer))
    {
        printf("Failed to run timer for wifi!\n");

    }     

    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        
    }
    cyw43_arch_enable_sta_mode();

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms("Your Wi-Fi SSID", "Your Wi-Fi Password", CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        
    } else {
        printf("Connected.\n");
        uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }


    while(1)
    {
         if(flag_wifi==true)
        {
            flag_wifi = false;
            printf("Serviced Wifi!\n");
        }
        sleep_ms(10000);
    }
}