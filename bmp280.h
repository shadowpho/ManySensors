#pragma once

#include <cstdint>

constexpr uint8_t BMP_ADDR = 0x76;

bool init_BMP280(); //must wait >5ms after calling this

bool start_auto_BMP280();

bool get_data_BMP280(float* temp, float* return_pressure);