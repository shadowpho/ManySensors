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

uint8_t __buff_special[3]; 
#define READ_VEML7700(register_address, recv_buff, num_of_bytes) assert(true == read_from_1byte_register(VEML7700_ADDRESS, register_address, (uint8_t *)recv_buff, num_of_bytes))
#define WRITE_VEML7700(register_address, send_buff, num_of_bytes)                                                 \
    do                                                                                                            \
    {                                                                                                             \
        assert(num_of_bytes==2);  \
        __buff_special[0]=register_address;\
        __buff_special[1]=((uint8_t*)send_buff)[0]; \
        __buff_special[2]=((uint8_t*)send_buff)[1];\
        assert(true == write_to_device(VEML7700_ADDRESS,__buff_special,3));\
    } while (0)

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

const float OH_MULT = 1.3 * 1.2;
                               //39          78           156            312        624          1286
const float VEML_DELAY_TIME[] = {25*OH_MULT, 50*OH_MULT, 100*OH_MULT, 200*OH_MULT, 400*OH_MULT, 800*OH_MULT};

void veml7700_dump_registers()
{
    uint16_t buff;
    for (int i = 0; i < 7; i++)
    {
        READ_VEML7700(i, &buff, 2);
        printf("Reg#%i:%u\n", i, buff);
    }
}
// Use 3,4,1,2 for gain, -2->3 for IT
// Returns how long to sleep for
uint32_t VEML_Start_Single_Measurment(int8_t gain, int8_t integration)
{
    static uint16_t CONF_REGISTER = 0x1;
    uint16_t als_count = 0;
    assert(gain > 0);
    assert(gain <= 4);
    assert(integration >= -2);
    assert(integration <= 3);
    CONF_REGISTER |= 0x1;
    WRITE_VEML7700(VEML_CONF_REGISTER, &CONF_REGISTER, 2);                                  // SHUT DOWN
    CONF_REGISTER = ALS_INT_VALUE[integration + 2] << 6 | ALS_GAIN_VALUE[gain] << 11 | 0x1; // write new value with SD

    WRITE_VEML7700(VEML_CONF_REGISTER, &CONF_REGISTER, 2); // set gain/integration

    CONF_REGISTER &= ~1;                                   // enable
    WRITE_VEML7700(VEML_CONF_REGISTER, &CONF_REGISTER, 2); // GOGOGO
    int delay_time_veml = VEML_DELAY_TIME[integration + 2];
    return (int)(3.5 +delay_time_veml); // 30% buffer/tolerance + 2.5ms + 0.5ms to round
}

uint32_t VEML_Read_Single_Measurment(float *lux, const int8_t gain, const int8_t integration)
{
    uint16_t als_count = 0;
    uint16_t CONF_REGISTER = ALS_INT_VALUE[integration + 2] << 6 | ALS_GAIN_VALUE[gain] << 11 | 0x1;
    assert(lux != NULL);
    assert(gain > 0);
    assert(gain <= 4);
    assert(integration >= -2);
    assert(integration <= 3);
    READ_VEML7700(VEML_ALS_Data, &als_count, 2);
    *lux = (float)als_count;
    //WRITE_VEML7700(VEML_CONF_REGISTER, &CONF_REGISTER,2);//finish transaction and sleep
    //int delay_time_veml = VEML_DELAY_TIME[integration + 2];
    //sleep_ms((int)3.5 + delay_time_veml);    
    // WRITE_VEML7700(VEML_CONF_REGISTER, &buff, 2);
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
    *lux *= 0.0042;
    printf("L:%.0f,G:%i,I:%i,R:%i\n", *lux, gain, integration, als_count);
    return als_count;
}

bool init_VEML7700()
{
    uint16_t buff = 0;
    READ_VEML7700(VEML_REGISTERS::VEML_ID, &buff, 2);
    // on bus we have 0x81, 0xC4,  on this device they FLIP we get 0xC4, 0x81
    if (buff != 0xC481)
    {
        printf("did not detect VEML! %i\n", buff);
        return false;
    }
    buff = 0x1; // ALS shut down
    WRITE_VEML7700(VEML_REGISTERS::VEML_CONF_REGISTER, &buff, 2);
    buff = 0x0; // Power Savings disabled
    sleep_ms(3);
    WRITE_VEML7700(VEML_REGISTERS::VEML_PS, &buff, 2);
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

// 0 = success, data is ready. call_in_ms = total time it took to get data
//-1 = call function again after call_in_ms
//-2 = general failure

int process_VEML7700(float *lux, uint32_t *call_in_ms)
{
    assert(lux != NULL);
    assert(call_in_ms != NULL);

    static int gain = 1;
    static enum VEML_STATE ALS_current_state = VEML_STATE::INITIALIZE;
    static int integration = 0;
    static absolute_time_t starting_measurment_time;
    static int ALS_ret_count = 0;

change_state:
    switch (ALS_current_state)
    {
    case VEML_STATE::INITIALIZE:
        gain = 1;
        integration = 0;
        *call_in_ms = VEML_Start_Single_Measurment(gain, integration);
        ALS_current_state = VEML_STATE::GET_INIT_RESULTS;
        starting_measurment_time = get_absolute_time();
        return -1;
    case VEML_STATE::GET_INIT_RESULTS:
        ALS_ret_count = VEML_Read_Single_Measurment(lux, gain, integration);
        if (ALS_ret_count <= 100)
        {
            ALS_current_state = VEML_STATE::DARK_LOOP;
            goto change_state;
        }
        else if (ALS_ret_count > 10000)
        {
            ALS_current_state = VEML_STATE::BRIGHT_LOOP;
            goto change_state;
        }
        else
        {
            goto success;
        }
    case VEML_STATE::DARK_LOOP:
        gain++;
        if (gain > 4) // maxed out gain
        {
            gain = 4;
            integration++;
            if (integration >= 4)
                goto success; // MAXED OUT TIME AND GAIN
            *call_in_ms = VEML_Start_Single_Measurment(gain, integration);
            ALS_current_state = VEML_STATE::DARK_LOOP_RESULTS;
            return -1;
        }
        *call_in_ms = VEML_Start_Single_Measurment(gain, integration);
        ALS_current_state = VEML_STATE::DARK_LOOP_RESULTS;
        return -1;
    case VEML_STATE::DARK_LOOP_RESULTS:
        ALS_ret_count = VEML_Read_Single_Measurment(lux, gain, integration);
        if (ALS_ret_count > 100)
            goto success;
        ALS_current_state = VEML_STATE::DARK_LOOP;
        goto change_state;
    case VEML_STATE::BRIGHT_LOOP:
        integration--;
        if (integration <= -3) // maxed out IT
        {
            goto success; // MAXED OUT TIME AND GAIN
        }
        *call_in_ms = VEML_Start_Single_Measurment(gain, integration);
        ALS_current_state = VEML_STATE::BRIGHT_LOOP_RESULTS;
        return -1;
    case VEML_STATE::BRIGHT_LOOP_RESULTS:
        ALS_ret_count = VEML_Read_Single_Measurment(lux, gain, integration);
        if (ALS_ret_count <= 10000)
            goto success;
        ALS_current_state = VEML_STATE::BRIGHT_LOOP;
        goto change_state;
    default:
        assert(false);
    }
success:
    absolute_time_t final_measurment_time = get_absolute_time();
    *call_in_ms = absolute_time_diff_us(starting_measurment_time, final_measurment_time) / 1000;
    ALS_current_state = VEML_STATE::INITIALIZE;
    float lux_calc = *lux;
    if (lux_calc > 1000)
        *lux = lux_calc * (1.0023 + lux_calc * (0.000081488 + lux_calc * (-9.3924e-9 + 6.0135e-13 * lux_calc)));
    return 0;
}