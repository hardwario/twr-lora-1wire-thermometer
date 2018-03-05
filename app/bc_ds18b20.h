#ifndef _BC_DS18B20_H
#define _BC_DS18B20_H

#include <bc_tick.h>
#include <bc_module_sensor.h>
#include <bc_scheduler.h>

//! @addtogroup bc_ds18b20 bc_ds18b20
//! @brief Driver for 1wire ds18b20
//! @{

//! @brief Callback events

typedef enum
{
    //! @brief Error event
    bc_ds18b20_EVENT_ERROR = -1,

    //! @brief Update event
    bc_ds18b20_EVENT_UPDATE = 0

} bc_ds18b20_event_t;

//! @brief BigClown ds18b20 instance

typedef struct bc_ds18b20_t bc_ds18b20_t;


//! @cond

typedef enum
{
    BC_DS18B20_STATE_ERROR = -1,
    BC_DS18B20_STATE_PREINITIALIZE = 0,
    BC_DS18B20_STATE_INITIALIZE = 1,
    BC_DS18B20_STATE_READY = 2,
    BC_DS18B20_STATE_MEASURE = 3,
    BC_DS18B20_STATE_READ = 4,
    BC_DS18B20_STATE_UPDATE = 5

} bc_ds18b20_state_t;

typedef enum
{
    BC_DS18B20_RESOLUTION_BITS_9 = 0,
    BC_DS18B20_RESOLUTION_BITS_10 = 1,
    BC_DS18B20_RESOLUTION_BITS_11 = 2,
    BC_DS18B20_RESOLUTION_BITS_12 = 3

} bc_ds18b20_resolution_bits_t;

struct bc_ds18b20_t
{
    bc_scheduler_task_id_t _task_id_interval;
    bc_scheduler_task_id_t _task_id_measure;
    void (*_event_handler)(bc_ds18b20_t *, bc_ds18b20_event_t, void *);
    void *_event_param;
    bool _measurement_active;
    bc_tick_t _update_interval;
    bc_ds18b20_state_t _state;
    int16_t _temperature_raw;
    uint64_t _device_number;
    bool _temperature_valid;
    bc_ds18b20_resolution_bits_t _resolution;
    bool _power;
};

//! @endcond


//! @brief Initialize ds18b20
//! @param[in] self Instance

void bc_ds18b20_init(bc_ds18b20_t *self, bc_ds18b20_resolution_bits_t resolution);

//! @brief Set callback function
//! @param[in] self Instance
//! @param[in] event_handler Function address
//! @param[in] event_param Optional event parameter (can be NULL)

void bc_ds18b20_set_event_handler(bc_ds18b20_t *self, void (*event_handler)(bc_ds18b20_t *, bc_ds18b20_event_t, void *), void *event_param);

//! @brief Set measurement interval
//! @param[in] self Instance
//! @param[in] interval Measurement interval

void bc_ds18b20_set_update_interval(bc_ds18b20_t *self, bc_tick_t interval);

//! @brief Start measurement manually
//! @param[in] self Instance
//! @return true On success
//! @return false When other measurement is in progress

bool bc_ds18b20_measure(bc_ds18b20_t *self);

//! @brief Get measured temperature in degrees of Celsius
//! @param[in] self Instance
//! @brief Get measured temperature as raw values
//! @return true When value is valid
//! @return false When value is invalid

bool bc_ds18b20_get_temperature_raw(bc_ds18b20_t *self, int16_t *raw);

//! @brief Get measured temperature in degrees of Celsius
//! @param[in] self Instance
//! @param[in] celsius Pointer to variables where results will be stored
//! @return true When value is valid
//! @return false When value is invalid

bool bc_ds18b20_get_temperature_celsius(bc_ds18b20_t *self, float *celsius);

//! @}

#endif // _BC_DS18B20_H
