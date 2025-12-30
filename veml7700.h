#pragma once

#include <cstdint>

constexpr uint8_t VEML7700_ADDRESS = 0x10;

bool init_VEML7700();

bool start_auto_VEML7700();

bool get_data_VEML7700(float *light);