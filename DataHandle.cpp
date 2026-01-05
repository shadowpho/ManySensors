#include "DataHandle.h"


CMA_Data sensor_CMA_data[(uint8_t)SENSORS::NUM_OF_ELEMENTS];

void add_to_CMA(CMA_Data* struct_data, float val1, float val2, float val3, float val4)
{
  struct_data->CMA_value1 += val1;
  struct_data->CMA_value2 += val2;
  struct_data->CMA_value3 += val3;
  struct_data->CMA_value4 += val4;
  struct_data->num_of_samples++;
}
void remove_CMA(CMA_Data* struct_data, float* val1, float* val2, float* val3, float* val4)
{
  float ret_value1 = 0;
  float ret_value2 = 0;
  float ret_value3 = 0;
  float ret_value4 = 0;
  if (struct_data->num_of_samples != 0) {
    ret_value1 = struct_data->CMA_value1 / struct_data->num_of_samples;
    ret_value2 = struct_data->CMA_value2 / struct_data->num_of_samples;
    ret_value3 = struct_data->CMA_value3 / struct_data->num_of_samples;
    ret_value4 = struct_data->CMA_value4 / struct_data->num_of_samples;
  } else {
    ret_value1 = INVALID_VALUE;
    ret_value2 = INVALID_VALUE;
    ret_value3 = INVALID_VALUE, ret_value4 = INVALID_VALUE;
  }
  if (val1 != nullptr) *val1 = ret_value1;
  if (val2 != nullptr) *val2 = ret_value2;
  if (val3 != nullptr) *val3 = ret_value3;
  if (val4 != nullptr) *val4 = ret_value4;
  struct_data->num_of_samples = 0;
  struct_data->CMA_value1 = 0;
  struct_data->CMA_value2 = 0;
  struct_data->CMA_value3 = 0;
  struct_data->CMA_value4 = 0;
}