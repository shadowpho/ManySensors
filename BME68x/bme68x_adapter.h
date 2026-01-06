#pragma once

#include "i2c.h"

//(*bme68x_read_fptr_t)
signed char user_i2c_read(unsigned char reg_addr, unsigned char *reg_data, unsigned int length, void *intf_ptr)
{
    bool ret_val = read_from_1byte_register(BME68X_I2C_ADDR_HIGH, reg_addr, reg_data, length);
    if (ret_val == true)
        return 0;
    return -1;
}

signed char user_i2c_write(unsigned char reg_addr, const unsigned char *reg_data, unsigned int length, void *intf_ptr)
{
    bool ret_val = write_to_register(BME68X_I2C_ADDR_HIGH, reg_addr, reg_data, length);
    if (ret_val == true)
        return 0;
    return -1;
}

void user_delay_us(uint32_t period, void *intf_ptr)
{
    sleep_us(period);
}
