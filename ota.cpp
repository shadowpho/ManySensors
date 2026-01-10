#include "ota.h"
#include "myconfig.h"

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <pico/sha256.h>
#include "pico/bootrom.h"
#include "boot/picobin.h"
#include "boot/picoboot.h"
#include <boot/uf2.h>

#include <string.h>
#include <stdlib.h>

#define WORKAREA_SIZE (4*1024)
#define FLASH_SECTOR_ERASE_SIZE 4096u


typedef struct TCP_UPDATE_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    struct tcp_pcb *client_pcb;
    bool complete;
    __attribute__((aligned(4))) uint8_t buffer_sent[SHA256_RESULT_BYTES];
    __attribute__((aligned(4))) uint8_t buffer_recv[2048];
    int sent_len;
    int recv_len;
    int num_blocks;
    int blocks_done;
    uint32_t family_id;
    uint32_t flash_update;
    int32_t write_offset;
    uint32_t write_size;
    uint32_t highest_erased_sector;
} TCP_UPDATE_SERVER_T;

TCP_UPDATE_SERVER_T *state; 

void init_ota()
{
    state = (TCP_UPDATE_SERVER_T*)calloc(1, sizeof(TCP_UPDATE_SERVER_T));
     if (!state) {
        printf("failed to allocate state\n");
        return;
    }
    return;
}
void check_accept_new_partition()
{
    uint8_t *workarea = (uint8_t*)aligned_alloc(4, WORKAREA_SIZE);
    
    boot_info_t boot_info = {}; 
    int ret = rom_get_boot_info(&boot_info);
    printf("Boot partition was %d\n", boot_info.partition);

    if (rom_get_last_boot_type() == BOOT_TYPE_FLASH_UPDATE) {
        printf("Someone updated into me\n");
        if (boot_info.reboot_params[0]) printf("Flash update base was %x\n", boot_info.reboot_params[0]);
        if (boot_info.tbyb_and_update_info) printf("Update info %x\n", boot_info.tbyb_and_update_info);
        ret = rom_explicit_buy(workarea, WORKAREA_SIZE);
        if (ret) printf("Buy returned %d\n", ret);
        ret = rom_get_boot_info(&boot_info);
        if (boot_info.tbyb_and_update_info) printf("Update info now %x\n", boot_info.tbyb_and_update_info);
    }
}

typedef struct uf2_block uf2_block_t;

void exec_OTA(void)
{
    resident_partition_t uf2_target_partition;
    uf2_block_t* block;
    int i=0;
    block = (uf2_block_t*)(state->buffer_recv + i * sizeof(uf2_block_t));
    uint16_t first_sector_number = (uf2_target_partition.permissions_and_location & PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_BITS) >> PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB;
    uint16_t last_sector_number = (uf2_target_partition.permissions_and_location & PICOBIN_PARTITION_LOCATION_LAST_SECTOR_BITS) >> PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB;
    uint32_t code_start_addr = first_sector_number * 0x1000;
    uint32_t code_end_addr = (last_sector_number + 1) * 0x1000;
    uint32_t code_size = code_end_addr - code_start_addr;
    printf("Start %lx, End %lx, Size %lx\n", code_start_addr, code_end_addr, code_size);

    state->flash_update = code_start_addr + XIP_BASE;
    state->write_offset = code_start_addr + XIP_BASE - block->target_addr;
    state->write_size = code_size;
    printf("Write Offset %lx, Size %lx\n", state->write_offset, state->write_size);
    int ret = rom_reboot(REBOOT2_FLAG_REBOOT_TYPE_FLASH_UPDATE, 3000, state->flash_update, 0);
    printf("Done - rebooting for a flash update boot %d\n", ret);
    sleep_ms(10000);
}