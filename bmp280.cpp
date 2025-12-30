#include "bmp280.h"
#include "i2c.h"
#include <stdio.h>

#define READ_BMP280(register_address, recv_buff, num_of_bytes) \
  assert(true == read_from_1byte_register(BMP_ADDR, (uint8_t)register_address, (uint8_t*)recv_buff, num_of_bytes))
#define WRITE_BMP280(register_address, src_buff, num_of_bytes) \
  assert(true == write_to_register(BMP_ADDR, (uint8_t)register_address, (const uint8_t*)src_buff, num_of_bytes))

enum BMP280_REGISTERS {
BMP280_CHIP_ID = 0xD0,
BMP280_RESET =0xE0,
BMP280_STATUS =0xF3,
BMP280_CTRL_MEAS =0xF4,
BMP280_CONFIG =0xF5,
BMP280_PRES_MSB =0xF7,
BMP280_PRES_LSB =0xF8,
BMP280_PRES_XLSB =0xF9,
BMP280_TEMP_MSB =0xFA,
BMP280_TEMP_LSB =0xFB,
BMP280_TEMP_XLSB =0xFC,
BMP280_COMP = 0x88
};

struct
{
  uint16_t dig_t1; // 0x88 and 0x89
  int16_t dig_t2;
  int16_t dig_t3;
  uint16_t dig_p1;
  int16_t dig_p2;
  int16_t dig_p3;
  int16_t dig_p4;
  int16_t dig_p5;
  int16_t dig_p6;
  int16_t dig_p7;
  int16_t dig_p8;
  int16_t dig_p9;
} compensation;

bool setup_BMP280()
{
  uint8_t buff[2];
  READ_BMP280(BMP280_REGISTERS::BMP280_CHIP_ID, buff, 1);
  if (buff[0] != 0x58) {
    printf("Wrong device ID for BMP280! %i\n", buff[0]);
    return false;
  }
  READ_BMP280(BMP280_REGISTERS::BMP280_COMP, &compensation, sizeof(compensation));

  buff[1] = 0xB6; // SOFT RESET, forces sensor into sleep mode (allows config writes)
  WRITE_BMP280(BMP280_REGISTERS::BMP280_RESET, buff, 1);
  return true;
}

bool start_auto_BMP280()
{
    uint8_t buff[2];
    //osrs_p=x16=111, osrs_t=x4=011, 40ms nominal, max of 48ms.
    //IIR=x4=011=3,
    //standby_time=t_sb=1000ms=101=0x5, effective 1.05s to run
    buff[0] = 0x5<<5 | 0x3<<2; 
    WRITE_BMP280(BMP280_REGISTERS::BMP280_CONFIG, buff, 1);
    buff[0] = 0x3<<5 | 0x7<<2 | 0x3;
    WRITE_BMP280(BMP280_REGISTERS::BMP280_CTRL_MEAS, buff, 1);
    return true;
}

bool get_data_BMP280(float* temp, float* return_pressure)
{
  uint8_t buff[6];
  uint32_t uncomp_press;
  int32_t uncomp_temp;
  int32_t temperature;
  uint32_t pressure;
  int32_t t_fine;

  assert(temp != nullptr);
  assert(return_pressure != nullptr);

  READ_BMP280(BMP280_PRES_MSB, buff, 6);
  uncomp_press = (uint32_t)buff[0] << 12 | (uint32_t)buff[1] << 4 | (uint32_t)buff[2] >> 4;
  uncomp_temp = (int32_t)buff[3] << 12 | (int32_t)buff[4] << 4 | (int32_t)buff[5] >> 4;
  {
    int32_t var1, var2;
    var1 = ((((uncomp_temp / 8) - ((int32_t)compensation.dig_t1 * 2))) * ((int32_t)compensation.dig_t2)) / 2048;
    var2 = (((((uncomp_temp / 16) - ((int32_t)compensation.dig_t1)) *
              ((uncomp_temp / 16) - ((int32_t)compensation.dig_t1))) /
             4096) *
            ((int32_t)compensation.dig_t3)) /
           16384;

    t_fine = var1 + var2;

    temperature = (t_fine * 5 + 128) / 256;
  }
  {
    int64_t var1, var2, p;

    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)compensation.dig_p6;
    var2 = var2 + ((var1 * (int64_t)compensation.dig_p5) * 131072);
    var2 = var2 + (((int64_t)compensation.dig_p4) * (int64_t)34359738368);
    var1 = ((var1 * var1 * (int64_t)compensation.dig_p3) / 256) + ((var1 * (int64_t)compensation.dig_p2) * 4096);
    var1 = (((((int64_t)1) * (int64_t)140737488355328) + var1)) * ((int64_t)compensation.dig_p1) / (int64_t)8589934592;

    if (var1 != 0) {
      p = 1048576 - uncomp_press;
      p = (((p * 2147483648) - var2) * 3125) / var1;
      var1 = (((int64_t)compensation.dig_p9) * (p / 8192) * (p / 8192)) / 33554432;
      var2 = (((int64_t)compensation.dig_p8) * p) / 524288;

      p = ((p + var1 + var2) / 256) + (((int64_t)compensation.dig_p7) * 16);
      pressure = (uint32_t)p;
    }
  }
  *return_pressure = (float)pressure / 256.0;
  *temp = ((float)temperature) / 100.0;
  return true;
}