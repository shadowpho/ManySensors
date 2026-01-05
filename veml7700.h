#pragma once

#include <cstdint>

constexpr uint8_t VEML7700_ADDRESS = 0x10;

bool init_VEML7700();

//0 = all good, data ready to use. 
//-1 = call again after call_in_ms milliseconds
//-2 = error
int process_VEML7700(float *light, uint32_t* call_in_ms);