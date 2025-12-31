#include "veml7700.h"
#include "i2c.h"


#define VEML_CONF_REGISTER 0x0
#define VEML_PS 0x1
#define VEML_ALS_Data 0x4
#define VEML_White_Data 0x5
#define VEML_INT 0x6

#define READ_VEML7700(register_address, recv_buff, num_of_bytes) assert(true == read_from_1byte_register(VEML7700_ADDR, register_address, (uint8_t *)recv_buff, num_of_bytes))
#define WRITE_VEML7700(register_address, recv_buff, num_of_bytes) assert(true == write_to_register(VEML7700_ADDR,  register_address, (uint8_t *)recv_buff, num_of_bytes))

/* From DS:
ALS integration time setting IT:
1100 = 25 ms → -2
1000 = 50 ms → -1
0000 = 100 ms → 0
0001 = 200 ms → 1
0010 = 400 ms → 2
0011 = 800 ms → 3
Gain selection G:
00 = ALS gain x 1 → 3
01 = ALS gain x 2 → 4
10 = ALS gain x (1/8) → 1
11 = ALS gain x (1/4) → 2
*/
enum VEML_IT_TIME
{
    VEML_25MS = -2,
    VEML_50MS = -1,
    VEML_100MS = 0,
    VEML_200MS = 1,
    VEML_400MS = 2,
    VEML_800MS = 3
};

//                                0    1     2   3   4   5
const uint8_t ALS_INT_VALUE[6] = {0xC, 0x8, 0x0, 0x1, 0x2, 0x3};
//                           GAIN - NA,1(1/8)2(1/4)3(x1)4(x2)
const uint8_t ALS_GAIN_VALUE[5] = {0x0, 0x2, 0x3, 0x0, 0x1};

#define OH_time 60
#define OH_MULT 1.2
const int VEML_DELAY_TIME[] = {60, 110, 200, 350, 700, 1200};