#pragma once

#include "BME68x/bme68x.h"


int BSEC_BME_init();

int BSEC_BME_loop(float *temp, float *pressure, float *humidity, float *VOC);

int BSEC_BME_selftest();

int BSEC_desired_sleep_us();
