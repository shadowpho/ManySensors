#include "PMS7003.h"
#include <cassert>

#include "uart.h"


#define MAGIC1 0x42
#define MAGIC2 0x4D

#define PMS7003_BUF_LENGTH 32

typedef enum
{
    BEGIN1,
    BEGIN2,
    LENGTH1,
    LENGTH2,
    DATA,
    CHECK1,
    CHECK2
} EState;

typedef struct
{
    EState state;
    uint8_t buf[PMS7003_BUF_LENGTH];
    const int size = PMS7003_BUF_LENGTH;
    int idx, len;
    uint16_t chk, sum;
} TState;

static TState state;


void setup_PMS7003()
{
    state.state = BEGIN1;
    state.idx = state.len = 0;
    state.chk = state.sum = 0;
}

static uint16_t get(uint8_t *buf, int idx)
{
    uint16_t data;
    data = buf[idx] << 8;
    data += buf[idx + 1];
    return data;
}

bool PmsProcess(uint8_t b)
{
    switch (state.state)
    {
    // wait for BEGIN1 byte
    case BEGIN1:
        if (b == MAGIC1)
        {
            state.sum = b;
            state.state = BEGIN2;
        }
        break;
    // wait for BEGIN2 byte
    case BEGIN2:
        if (b == MAGIC2)
        {
            state.sum += b;
            state.state = LENGTH1;
        }
        else
        {
            state.state = BEGIN1;
            // retry
            return PmsProcess(b);
        }
        break;
    // verify data length
    case LENGTH1:
        state.sum += b;
        state.len = b << 8;
        state.state = LENGTH2;
        break;
    case LENGTH2:
        state.sum += b;
        state.len += b;
        state.len -= 2; // exclude checksum bytes
        if (state.len <= state.size && state.len > 0)
        {
            state.idx = 0;
            state.state = DATA;
        }
        else
        {
            // bogus length
            state.state = BEGIN1;
        }
        break;
    // store data
    case DATA:
        state.sum += b;
        state.buf[state.idx++] = b;
        if (state.idx == state.len)
        {
            state.state = CHECK1;
        }
        break;
    // store checksum
    case CHECK1:
        state.chk = b << 8;
        state.state = CHECK2;
        break;
    // verify checksum
    case CHECK2:
        state.chk += b;
        state.state = BEGIN1;
        return state.chk == state.sum;
    }
    return false;
}

bool read_from_PMS(uint16_t *pm10, uint16_t *pm25, uint16_t *pm1)
{
    //XXX average if there's >1x 32 byte available
    while(uart_rx_available())
        if (PmsProcess(uart_rx_get()))
        {
            *pm10 = get(state.buf, 4);
            *pm25 = get(state.buf, 2);
            *pm1  = get(state.buf, 0);
            return true ;
        }

    return false; //not ready yet    
}
