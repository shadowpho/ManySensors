#include "veml7700.h"
#include "i2c.h"
#include "stdio.h"
#include "pico/time.h"

enum VEML_REGISTERS
{
    VEML_CONF_REGISTER = 0x0,
    VEML_PS = 0x3,
    VEML_ALS_Data = 0x4,
    VEML_White_Data = 0x5,
    VEML_INT = 0x6,
    VEML_ID = 0x7
};

#define READ_VEML7700(register_address, recv_buff, num_of_bytes) assert(true == read_from_1byte_register(VEML7700_ADDRESS, register_address, (uint8_t *)recv_buff, num_of_bytes))
#define WRITE_VEML7700(register_address, recv_buff, num_of_bytes) assert(true == write_to_register(VEML7700_ADDRESS, register_address, (uint8_t *)recv_buff, num_of_bytes))

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

// Use 3,4,1,2 for gain, -2->3 for IT
// Returns how long to sleep for
uint32_t VEML_Start_Single_Measurment(int8_t gain, int8_t integration)
{
    uint16_t buff = 0x1;
    uint16_t als_count = 0;
    assert(gain > 0);
    assert(gain <= 4);
    assert(integration >= -2);
    assert(integration <= 3);

    WRITE_VEML7700(VEML_CONF_REGISTER, &buff, 2); // SHUT DOWN
    buff = ALS_INT_VALUE[integration + 2] << 6 | ALS_GAIN_VALUE[gain] << 11 | 0x1;
    WRITE_VEML7700(VEML_CONF_REGISTER, &buff, 2); // set gain/integration
    buff &= ~1;
    WRITE_VEML7700(VEML_CONF_REGISTER, &buff, 2); // GOGOGO
    int delay_time_veml = VEML_DELAY_TIME[integration + 2];
    return (int)((float)delay_time_veml * 1.2); // 20% buffer/tolerance
}

uint32_t VEML_Read_Single_Measurment(float *lux, int8_t gain, int8_t integration)
{
    uint16_t buff = 0x1;
    uint16_t als_count = 0;
    assert(gain > 0);
    assert(gain <= 4);
    assert(integration >= -2);
    assert(integration <= 3);

    READ_VEML7700(VEML_ALS_Data, &als_count, 2);
    // printf("Lux!G:%i,I:%i,R:%i\n", gain, integration, als_count);
    *lux = (float)als_count;
    buff = 0x1; // shutdown
    WRITE_VEML7700(VEML_CONF_REGISTER, &buff, 2);
    switch (gain)
    {
    case 1:
        *lux *= 16;
        break; // 1/8
    case 2:
        *lux *= 8;
        break; // 1/4
    case 3:
        *lux *= 2;
        break; // x1
    case 4:
        break; // x2
    default:
        break;
    }
    // integration is -2 -> 3.  We turn it to 0->5 (+2)
    // 0 = 25ms, 5 = 800ms
    // 0= *32, 1 = *16... 5=*1
    //(5-(integration+2))
    *lux *= 1 << (5 - (integration + 2));
    *lux *= 0.0036;
    return als_count;
}

bool init_VEML7700()
{
    uint8_t buff[2] = {0};
    READ_VEML7700(VEML_REGISTERS::VEML_ID, buff, 2);
    if(!((buff[0]==129)&&(buff[1]==196)))
    {
        printf("did not detect VEML! %i,%i",buff[0],buff[1]);
        return false;
    }
    buff[0] = 0x1; // ALS shut down
    WRITE_VEML7700(VEML_REGISTERS::VEML_CONF_REGISTER, buff, 2);
    buff[0] = 0x0; // Power Savings disabled
    sleep_ms(3);
    WRITE_VEML7700(VEML_REGISTERS::VEML_PS, buff, 2);
    VEML_Start_Single_Measurment(1, 0);
    return true;
}

enum class VEML_STATE
{
    INITIALIZE,
    GET_INIT_RESULTS,
    DARK_LOOP,
    DARK_LOOP_RESULTS,
    BRIGHT_LOOP,
    BRIGHT_LOOP_RESULTS
};

// 0 = success, read it out fully. call_in_ms = total time it took
//-1 = call me again after call_in_ms
//-2 = general failure?

int process_VEML7700(float *lux, uint32_t *call_in_ms)
{
    assert(lux != NULL);
    assert(call_in_ms != NULL);

    static int gain = 1;
    static int ALS_current_state = 0;
    static int integration = 0;
    static absolute_time_t starting_measurment_time;
    static int ALS_ret_count = 0;

change_state:
    switch (ALS_current_state)
    {
    case (int)VEML_STATE::INITIALIZE:
        gain = 1;
        integration = 0;
        *call_in_ms = VEML_Start_Single_Measurment(gain, integration);
        ALS_current_state = (int)VEML_STATE::GET_INIT_RESULTS;
        starting_measurment_time = get_absolute_time();
        return -1;
    case (int)VEML_STATE::GET_INIT_RESULTS:
        ALS_ret_count = VEML_Read_Single_Measurment(lux, gain, integration);
        if (ALS_ret_count <= 100)
        {
            ALS_current_state = (int)VEML_STATE::DARK_LOOP;
            goto change_state;
        }
        else if (ALS_ret_count > 10000)
        {
            ALS_current_state = (int)VEML_STATE::BRIGHT_LOOP;
            goto change_state;
        }
        else
        {
            goto success;
        }
    case (int)VEML_STATE::DARK_LOOP:
        gain++;
        if (gain >= 4) // maxed out gain
        {
            gain = 4;
            integration++;
            if (integration >= 4)
                goto success; // MAXED OUT TIME AND GAIN
            *call_in_ms = VEML_Start_Single_Measurment(gain, integration);
            ALS_current_state = (int)VEML_STATE::DARK_LOOP_RESULTS;
            return -1;
        }
        *call_in_ms = VEML_Start_Single_Measurment(gain, integration);
        ALS_current_state = (int)VEML_STATE::DARK_LOOP_RESULTS;
        return -1;
    case (int)VEML_STATE::DARK_LOOP_RESULTS:
        ALS_ret_count = VEML_Read_Single_Measurment(lux, gain, integration);
        if (ALS_ret_count > 100)
            goto success;
        ALS_current_state = (int)VEML_STATE::DARK_LOOP;
        goto change_state;
    case (int)VEML_STATE::BRIGHT_LOOP:
        integration--;
        if (integration <= -3) // maxed out IT
        {            
            goto success; // MAXED OUT TIME AND GAIN
        }
        *call_in_ms = VEML_Start_Single_Measurment(gain, integration);
        ALS_current_state = (int)VEML_STATE::BRIGHT_LOOP_RESULTS;
        return -1;
    case (int)VEML_STATE::BRIGHT_LOOP_RESULTS:
        ALS_ret_count = VEML_Read_Single_Measurment(lux, gain, integration);
        if (ALS_ret_count <= 10000)
            goto success;
        ALS_current_state = (int)VEML_STATE::BRIGHT_LOOP;
        goto change_state;
    default:
        assert(false);
    }
success:
    absolute_time_t final_measurment_time = get_absolute_time();
    *call_in_ms = absolute_time_diff_us(final_measurment_time, starting_measurment_time) / 1000;
    ALS_current_state = 0;
    float lux_calc = *lux;
    if (lux_calc > 1000)
        *lux = lux_calc * (1.0023 + lux_calc * (0.000081488 + lux_calc * (-9.3924e-9 + 6.0135e-13 * lux_calc)));
    return 0;
}