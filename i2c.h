#pragma once

#include "hardware/i2c.h"

#define I2C_PORT i2c1
#define I2C_SDA 18
#define I2C_SCL 19

#define I2C_TIMEOUT 5000


void init_i2c();

bool write_to_device(uint8_t dev_address, const uint8_t *src, uint8_t len);
bool write_to_register(uint8_t dev_address,uint8_t dev_register, const uint8_t *src, uint8_t len);
bool read_from_2byte_register(uint8_t dev_address, uint16_t dev_register,  uint8_t *dst, size_t len);
bool read_from_1byte_register(uint8_t dev_address, uint8_t dev_register,  uint8_t *dst, size_t len);