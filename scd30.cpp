#include "scd30.h"

#include "i2c.h"
#include "stdio.h"
#include <cstring>

bool init_SCD30()
{
    uint8_t buff[5] = {0x0, 0x10, 0x00, 0x00, 0x81}; // cmd 16b, pressure16b, crc start continuous measurment
    int i = i2c_write_timeout_us(I2C_PORT, SCD30_ADDRESS, buff, 5, false, 200 * 1000);
    if (i != 5)
    {
        printf("failed to write to SCD30 to go\n");
        return false;
    }
    return true;
}

uint32_t read_u32_skip_crc(uint8_t* buf) {
  return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[3] << 8) | (uint32_t)buf[4];
}

bool get_data_SCD30(float *CO2, float *temp, float *hum)
{
    uint8_t buf[18] = {0x03, 0x00}; // GET DATA
    int i = i2c_write_timeout_us(I2C_PORT, SCD30_ADDRESS, buf, 2, false, 200 * 1000);
    if (i != 5)
    {
        printf("failed to get data from SCD30\n");
        return false;
    }
    sleep_ms(5);
    i = i2c_read_timeout_us(I2C_PORT, SCD30_ADDRESS, buf, 18, false, 200 * 1000);
    {
        printf("no data from SCD30\n");
        return false;
    }

    uint32_t co2U32 = read_u32_skip_crc(buf + 0);
    uint32_t tempU32 = read_u32_skip_crc(buf + 6);
    uint32_t humU32 = read_u32_skip_crc(buf + 12);

    if (CO2 != NULL)
        memcpy(CO2, &co2U32, sizeof(float));
    if (temp != NULL)
        memcpy(temp, &tempU32, sizeof(float));
    if (hum != NULL)
        memcpy(hum, &humU32, sizeof(float));
    return true;
}