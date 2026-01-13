#include "hdc302x.h"
#include "i2c.h"
#include <stdio.h>

#include "lwip/def.h"

enum HDC302x_Commands
{
  SOFT_RESET = 0x30A2,
  READ_MANUFACTURER_ID = 0x3781,
  READ_NIST_ID_SERIAL_BYTES_5_4 =
      0x3683, // Command to read bytes 5 and 4 of the NIST ID
  READ_NIST_ID_SERIAL_BYTES_3_2 =
      0x3684, // Command to read bytes 3 and 2 of the NIST ID
  READ_NIST_ID_SERIAL_BYTES_1_0 =
      0x3685,                             // Command to read bytes 1 and 0 of the NIST ID
  ACCESS_OFFSETS = 0xA004,                // Command to write/read offsets
  SET_HEATER_POWER = 0x306E,              // Command to set heater power
  ENABLE_HEATER = 0x306D,                 // Command to enable heater
  DISABLE_HEATER = 0x3066,                // Command to disable heater
  READ_STATUS_REGISTER = 0xF32D,          // Command to read the status register
  CLEAR_STATUS_REGISTER = 0x3041,         // Command to clear the status register
  MEASUREMENT_READOUT_AUTO_MODE = 0xE000, // Measurement Readout Auto Mode
  SET_LOW_ALERT = 0x6100,                 // Configure ALERT Thresholds for Set Low Alert
  SET_HIGH_ALERT = 0x611D,                // Configure ALERT Thresholds for Set High Alert
  CLR_LOW_ALERT = 0x610B,                 // Configure ALERT Thresholds for Clear Low Alert
  CLR_HIGH_ALERT = 0x6116                 // Configure ALERT Thresholds for Clear High Alert
};

typedef enum
{
  AUTO_MEASUREMENT_0_5MPS_LP0 = 0x2032,
  AUTO_MEASUREMENT_0_5MPS_LP1 = 0x2024,
  AUTO_MEASUREMENT_0_5MPS_LP2 = 0x202F,
  AUTO_MEASUREMENT_0_5MPS_LP3 = 0x20FF,
  AUTO_MEASUREMENT_1MPS_LP0 = 0x2130,
  AUTO_MEASUREMENT_1MPS_LP1 = 0x2126,
  AUTO_MEASUREMENT_1MPS_LP2 = 0x212D,
  AUTO_MEASUREMENT_1MPS_LP3 = 0x21FF,
  AUTO_MEASUREMENT_2MPS_LP0 = 0x2236,
  AUTO_MEASUREMENT_2MPS_LP1 = 0x2220,
  AUTO_MEASUREMENT_2MPS_LP2 = 0x222B,
  AUTO_MEASUREMENT_2MPS_LP3 = 0x22FF,
  AUTO_MEASUREMENT_4MPS_LP0 = 0x2334,
  AUTO_MEASUREMENT_4MPS_LP1 = 0x2322,
  AUTO_MEASUREMENT_4MPS_LP2 = 0x2329,
  AUTO_MEASUREMENT_4MPS_LP3 = 0x23FF,
  AUTO_MEASUREMENT_10MPS_LP0 = 0x2737,
  AUTO_MEASUREMENT_10MPS_LP1 = 0x2721,
  AUTO_MEASUREMENT_10MPS_LP2 = 0x272A,
  AUTO_MEASUREMENT_10MPS_LP3 = 0x27FF,
  EXIT_AUTO_MODE = 0x3093
} hdcAutoMode_t;

typedef enum
{
  TRIGGERMODE_LP0 = 0x2400, // Trigger-On Demand Mode, Low Power Mode 0
  TRIGGERMODE_LP1 = 0x240B, // Trigger-On Demand Mode, Low Power Mode 1
  TRIGGERMODE_LP2 = 0x2416, // Trigger-On Demand Mode, Low Power Mode 2
  TRIGGERMODE_LP3 = 0x24FF  // Trigger-On Demand Mode, Low Power Mode 3
} hdcTriggerMode_t;

typedef enum
{
  HEATER_OFF = 0x0000,
  HEATER_QUARTER_POWER = 0x009F,
  HEATER_HALF_POWER = 0x03FF,
  HEATER_FULL_POWER = 0x3FFF
} HDC302x_HeaterPower;

bool init_hdc302x()
{
  uint8_t buff[3];

  if (read_from_2byte_register(HDC3022_ADDRESS, htons(HDC302x_Commands::READ_MANUFACTURER_ID), (uint8_t *)&buff, 3) == false)
  {
    printf("failed to read from HDC3022x, attempting recovery\n");
    int i = i2c_read_blocking(I2C_PORT, HDC3022_ADDRESS, buff, 3, false);
    if (i < 0)
    {
      printf("Failed to re-read%i\n", i);
      return false;
    }
    if (read_from_2byte_register(HDC3022_ADDRESS, htons(HDC302x_Commands::READ_MANUFACTURER_ID), (uint8_t *)&buff, 3) == false)
      return false;
    else
      printf("Recovered HDC!!!!!!!!\n");
  }

  if (!((buff[0] == 0x30) && (buff[1] == 0x00)))
  {
    printf("hdc3022 mfg id wrong\n");
    return false;
  }
  uint16_t proper_command = htons(HDC302x_Commands::SOFT_RESET);
  if (write_to_device(HDC3022_ADDRESS, (const uint8_t *)&proper_command, 2) == false)
  {
    printf("hdc302x did not reset\n");
    return false;
  }
  return true;
}

bool start_auto_hdc302x()
{
  const uint16_t cmd = htons(AUTO_MEASUREMENT_0_5MPS_LP0);
  if (write_to_device(HDC3022_ADDRESS, (const uint8_t *)&cmd, 2) == false)
  {
    printf("hdc302x did not start auto\n");
    return false;
  }
  return true;
}

bool get_data_hdc302x(float *temperature, float *rh)
{
  uint8_t buffer[6];
  if (read_from_2byte_register(HDC3022_ADDRESS, htons(HDC302x_Commands::MEASUREMENT_READOUT_AUTO_MODE), buffer, 6))
  {
    printf("did not read data from HDC302x\n");
    return false;
  }
  uint16_t rawTemperature = (buffer[0] << 8) | buffer[1];
  uint16_t rawHumidity = (buffer[3] << 8) | buffer[4];

  *temperature = ((rawTemperature / 65535.0) * 175.0) - 45.0;
  *rh = (rawHumidity / 65535.0) * 100.0;
  return true;
}