#ifndef _BC_THERMISTOR_H
#define _BC_THERMISTOR_H

#include <bc_common.h>

void bc_thermistor_init(void);

bool bc_thermistor_get_temperature_celsius(float *celsius);

#endif // _BC_THERMISTOR_H
