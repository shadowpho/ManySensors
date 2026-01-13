#include "BSECglue.h"
#include <cassert>

#include "BME68x/bme68x_adapter.h"

#include "bsec/bsec_selectivity.h"
#include "bsec/bsec_interface.h"

#include <chrono>
#include <vector>
#include <cstring>

static struct bme68x_dev gas_sensor;
int64_t next_call_ns;

#define BME68x_NUM_OF_SENS 1
#define BSEC_INSTANCE_SIZE UINT16_C(3272)
#define BSEC_CHECK_INPUT(x, shift) (x & (1 << (shift - 1)))
#define BSEC_TOTAL_HEAT_DUR UINT16_C(140)

static struct bme68x_conf bme68x_config[BME68x_NUM_OF_SENS];
static struct bme68x_dev bme68x[BME68x_NUM_OF_SENS];
static struct bme68x_heatr_conf bme68x_heater_config[BME68x_NUM_OF_SENS];
// uint8_t	bsec_mem_block[BME68x_NUM_OF_SENS][BSEC_INSTANCE_SIZE];
uint8_t *bsecInstance[BME68x_NUM_OF_SENS];

#define CHK_RTN_BSEC(x)                         \
  do                                            \
  {                                             \
    int _rslt = (x);                            \
    if (_rslt != 0)                             \
    {                                           \
      printf("BSEC Error!%s, %d\n", #x, _rslt); \
      return _rslt;                             \
    }                                           \
  } while (0)
#define CHK_RTN_BME(x)                         \
  do                                           \
  {                                            \
    int _rslt = (x);                           \
    if (_rslt != 0)                            \
    {                                          \
      printf("BME Error!%s, %d\n", #x, _rslt); \
      return _rslt;                            \
    }                                          \
  } while (0)

int BSEC_BME_selftest()
{
  gas_sensor.intf = BME68X_I2C_INTF;
  gas_sensor.read = (bme68x_read_fptr_t)user_i2c_read;
  gas_sensor.write = (bme68x_write_fptr_t)user_i2c_write;
  gas_sensor.delay_us = user_delay_us;
  gas_sensor.amb_temp = 20;
  int rslt = bme68x_selftest_check(&gas_sensor);
  return rslt;
}

static int update_subscription(float sample_rate, uint8_t sens_no)
{
  int status;
  bsec_sensor_configuration_t requested_virtual_sensors[14];
  uint8_t n_requested_virtual_sensors = 14;

  bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
  uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

  requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_RAW_PRESSURE;
  requested_virtual_sensors[0].sample_rate = sample_rate;
  requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_RAW_TEMPERATURE;
  requested_virtual_sensors[1].sample_rate = sample_rate;
  requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_RAW_HUMIDITY;
  requested_virtual_sensors[2].sample_rate = sample_rate;
  requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_RAW_GAS;
  requested_virtual_sensors[3].sample_rate = sample_rate;
  requested_virtual_sensors[4].sensor_id = BSEC_OUTPUT_IAQ;
  requested_virtual_sensors[4].sample_rate = sample_rate;
  requested_virtual_sensors[5].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE;
  requested_virtual_sensors[5].sample_rate = sample_rate;
  requested_virtual_sensors[6].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY;
  requested_virtual_sensors[6].sample_rate = sample_rate;
  requested_virtual_sensors[7].sensor_id = BSEC_OUTPUT_STATIC_IAQ;
  requested_virtual_sensors[7].sample_rate = sample_rate;
  requested_virtual_sensors[8].sensor_id = BSEC_OUTPUT_CO2_EQUIVALENT;
  requested_virtual_sensors[8].sample_rate = sample_rate;
  requested_virtual_sensors[9].sensor_id = BSEC_OUTPUT_BREATH_VOC_EQUIVALENT;
  requested_virtual_sensors[9].sample_rate = sample_rate;
  requested_virtual_sensors[10].sensor_id = BSEC_OUTPUT_STABILIZATION_STATUS;
  requested_virtual_sensors[10].sample_rate = sample_rate;
  requested_virtual_sensors[11].sensor_id = BSEC_OUTPUT_RUN_IN_STATUS;
  requested_virtual_sensors[11].sample_rate = sample_rate;
  requested_virtual_sensors[12].sensor_id = BSEC_OUTPUT_GAS_PERCENTAGE;
  requested_virtual_sensors[12].sample_rate = sample_rate;

  n_requested_virtual_sensors = 13;

 //if (sample_rate == BSEC_SAMPLE_RATE_LP)
 // {
    requested_virtual_sensors[13].sensor_id = BSEC_OUTPUT_TVOC_EQUIVALENT;
    requested_virtual_sensors[13].sample_rate = sample_rate;
    n_requested_virtual_sensors = 14;
 // }
  status = bsec_update_subscription(bsecInstance[sens_no], requested_virtual_sensors, n_requested_virtual_sensors,
                                    required_sensor_settings, &n_required_sensor_settings);
  return status;
}

int BSEC_BME_init()
{
  next_call_ns = 0;
  uint8_t bsec_config[BSEC_MAX_PROPERTY_BLOB_SIZE] = {0};
  uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE] = {0};
  uint8_t bsec_state[BSEC_MAX_STATE_BLOB_SIZE] = {0};
  uint32_t bsec_state_len;

  for (uint8_t sens_no = 0; sens_no < BME68x_NUM_OF_SENS; sens_no++)
  {
    memset(work_buffer, 0, sizeof(work_buffer));
    memset(bsec_state, 0, sizeof(bsec_state));
    memset(bsec_config, 0, sizeof(bsec_config));
    memset(&bme68x[sens_no], 0, sizeof(bme68x[sens_no]));

    int ret_val = bme68x_init(&bme68x[sens_no]);
    
    assert(ret_val == 0);

    bsecInstance[sens_no] = (uint8_t *)malloc(BSEC_INSTANCE_SIZE);
    assert(bsecInstance[sens_no] != NULL);

    ret_val = bsec_init(bsecInstance[sens_no]);
    assert(ret_val == 0);

    ret_val = bsec_set_configuration(bsecInstance[sens_no], bsec_config_selectivity, BSEC_MAX_PROPERTY_BLOB_SIZE, work_buffer, sizeof(work_buffer));
    /* Load previous library state, if available */
    // bsec_state_len = state_load(bsec_state, sizeof(bsec_state));
    // ret_val = bsec_set_state(bsecInstance[sens_no], bsec_state, bsec_state_len, work_buffer, sizeof(work_buffer));
    ret_val = update_subscription(BSEC_SAMPLE_RATE_SCAN, sens_no);
  }

  return 0;
}

int BSEC_desired_sleep_us()
{
  using namespace std::chrono;
  int64_t now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
  if (next_call_ns <= now)
    return 0;
  return ((next_call_ns - now) / 1000) + 3;
}

int BSEC_BME_loop(float *temp, float *pressure, float *humidity, float *VOC)
{
  *temp = 0;
  *pressure = 0;
  *humidity = 0;
  *VOC = 0;
  using namespace std::chrono;
  int64_t now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
  if (now < next_call_ns)
  {
    printf("BSECglue: call too early %lli < %lli\n", now, next_call_ns);
    return -1; // too early
  }
  if (now >= next_call_ns + 1000000000)
  {
    printf("BSECglue: call too SLOW! %lli vs %lli\n", now, next_call_ns);
  }
 
  return 0;
}