#pragma once

#include <cstdint>
#include <math.h>

#ifndef INVALID_VALUE
#define INVALID_VALUE std::nanf("")
#endif


enum class SENSORS{
    pms7003,
    hdc_302x,
    scd30,
    bmp280,
    veml7700,
    bme688,
    NUM_OF_ELEMENTS
};

static constexpr const char* SENSORS_STRING[] = 
{
    "pms7003",
    "hdc_302x",
    "scd30",
    "bmp280",
    "veml7700",
    "bme688"
};

struct CMA_Data
{
  float CMA_value1 = 0; // moving average val1
  float CMA_value2 = 0; // moving average val2
  float CMA_value3 = 0; // moving average val3
  float CMA_value4 = 0; // moving average val3
  uint32_t num_of_samples = 0;
};

extern CMA_Data sensor_CMA_data[(uint8_t)SENSORS::NUM_OF_ELEMENTS];

void add_to_CMA(CMA_Data* struct_data, float val1, float val2, float val3, float val4);
void remove_CMA(CMA_Data* struct_data, float* val1, float* val2, float* val3, float* val4);

