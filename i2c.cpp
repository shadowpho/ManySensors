#include "i2c.h"

#include "pico/binary_info.h"
#include "hardware/gpio.h"


void init_i2c()
{
    bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));
    i2c_init(I2C_PORT, 100*1000); //100 kHz limit of SCD30
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

bool write_to_device(uint8_t dev_address, const uint8_t *src, uint8_t len)
{
    assert(len>0);
    assert(src!=NULL);
    int i = i2c_write_timeout_us(I2C_PORT, dev_address, src,len,false,I2C_TIMEOUT);
    if(i!=1) return false;

    return true;
}

bool read_from_2byte_register(uint8_t dev_address, uint16_t dev_register,  uint8_t *dst, size_t len)
{
    assert(len>0);
    assert(dst!=NULL);
    int i = i2c_write_timeout_us(I2C_PORT, dev_address, (const uint8_t*)&dev_register,2,true,I2C_TIMEOUT);
    if(i!=1) return false;
    i = i2c_read_timeout_us(I2C_PORT, dev_address,dst,len, false, I2C_TIMEOUT);
    if(i!=len) return false;

    return true;
}
bool read_from_1byte_register(uint8_t dev_address, uint8_t dev_register,  uint8_t *dst, size_t len)
{
    assert(len>0);
    assert(dst!=NULL);
    int i = i2c_write_timeout_us(I2C_PORT, dev_address, &dev_register,1,true,I2C_TIMEOUT);
    if(i!=1) return false;
    i = i2c_read_timeout_us(I2C_PORT, dev_address,dst,len, false, I2C_TIMEOUT);
    if(i!=len) return false;

    return true;
}