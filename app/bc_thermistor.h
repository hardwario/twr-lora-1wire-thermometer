#ifndef _BC_THERMISTOR_H
#define _BC_THERMISTOR_H

#include <bc_scheduler.h>
#include <bc_module_sensor.h>


typedef enum
{
    //! @brief Error event
    bc_thermistor_EVENT_ERROR = 0,

    //! @brief Update event
    bc_thermistor_EVENT_UPDATE = 1

} bc_thermistor_event_t;

typedef struct bc_thermistor_t bc_thermistor_t;

typedef enum
{
    BC_THERMISTOR_STATE_ERROR = -1,
    BC_THERMISTOR_STATE_INITIALIZE = 0,
    BC_THERMISTOR_STATE_MEASURE = 1,
    BC_THERMISTOR_STATE_MEASURE_RUN = 2,
    BC_THERMISTOR_STATE_READ = 3,
    BC_THERMISTOR_STATE_UPDATE = 4

} bc_thermistor_state_t;

struct bc_thermistor_t
{
    bc_module_sensor_channel_t _channel;
    bc_scheduler_task_id_t _task_id_interval;
    bc_scheduler_task_id_t _task_id_measure;
    void (*_event_handler)(bc_thermistor_t *, bc_thermistor_event_t, void *);
    void *_event_param;
    bool _measurement_active;
    bc_tick_t _update_interval;
    bc_thermistor_state_t _state;
    bool _temperature_valid;
    int32_t _raw;
};

void bc_thermistor_init(bc_thermistor_t *self, bc_module_sensor_channel_t channel);

void bc_thermistor_set_event_handler(bc_thermistor_t *self, void (*event_handler)(bc_thermistor_t *, bc_thermistor_event_t, void *), void *event_param);

void bc_thermistor_set_update_interval(bc_thermistor_t *self, bc_tick_t interval);

bool bc_thermistor_measure(bc_thermistor_t *self);

bool bc_thermistor_get_temperature_celsius(bc_thermistor_t *self, float *celsius);

#endif // _BC_THERMISTOR_H
