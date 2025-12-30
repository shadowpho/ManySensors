#pragma once

#include <cstdint>

constexpr uint8_t SCD30_ADDRESS = 0x61;

bool init_SCD30();

bool get_data_SCD30(float *CO2);