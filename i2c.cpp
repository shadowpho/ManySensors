#include "i2c.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"


void init_i2c()
{
    bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}