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
#include "malloc.h"

#include "GoodTimer.h"
#include "ota.h"

void core1_main()
{
    init_timers_core1();

    if (cyw43_arch_init())
    {
        printf("Wi-Fi init failed\n");
    }

    cyw43_arch_enable_sta_mode();

    check_accept_new_partition();
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(MYWIFI, MYWIFIPASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("failed to connect.\n");
    }
    else
    {
        printf("Connected.\n");
        uint8_t *ip_address = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
        printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }

    while (1)
    {
         uint32_t flags = std::atomic_exchange(&timer_flags_core1, 0);
        if (flags & (uint32_t)TIMER_FLAGS_CORE1::wifi)
        {
            int tcp_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
            int32_t rssi = 0;
            cyw43_wifi_get_rssi(&cyw43_state, &rssi);
            uint8_t bssid[6];
            cyw43_wifi_get_bssid(&cyw43_state, bssid);
            int wifi_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
            printf("WifiStats: Wifi:%i,TCP:%i,RSSI:%i,BSSID:%02X:%02X:%02X:%02X:%02X:%02X\n", wifi_status, tcp_status, rssi, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, tcp_status == CYW43_LINK_UP);
        }
        if (flags & (uint32_t)TIMER_FLAGS_CORE1::stats)
        {
            struct mallinfo info = mallinfo();
            printf("Total allocated: %d bytes\n", info.uordblks);
            printf("Total free: %d bytes\n", info.fordblks);
            printf("Total heap size: %d bytes\n", info.arena);
            printf("Largest free block: %d bytes\n", info.ordblks);
        }
        sleep_ms(10);
    }
}