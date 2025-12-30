#pragma once

#include <cstdint>

constexpr uint8_t HDC3022_ADDRESS = 0x44;

bool init_hdc302x();

bool start_auto_hdc302x();

bool get_data_hdc302x(float* temperature, float* rh);