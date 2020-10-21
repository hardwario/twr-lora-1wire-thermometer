#include <application.h>
#include <at.h>

/*

 SENSOR MODULE CONNECTION
==========================

Sensor Module R1.0 - 4 pin connector
VCC, GND, - , DATA

Sensor Module R1.1 - 5 pin connector
- , GND , VCC , - , DATA


 DS18B20 sensor pinout
=======================
VCC - red
GND - black
DATA- yellow (white)

*/

#define SEND_DATA_INTERVAL        (15 * 60 * 1000)
#define MEASURE_INTERVAL               (30 * 1000)

#define DS18B20_SENSOR_COUNT 10
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_0, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_1, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_2, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_3, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_4, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_5, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_6, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_7, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_8, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer_9, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))

bc_data_stream_t sm_temperature_0;
bc_data_stream_t sm_temperature_1;
bc_data_stream_t sm_temperature_2;
bc_data_stream_t sm_temperature_3;
bc_data_stream_t sm_temperature_4;
bc_data_stream_t sm_temperature_5;
bc_data_stream_t sm_temperature_6;
bc_data_stream_t sm_temperature_7;
bc_data_stream_t sm_temperature_8;
bc_data_stream_t sm_temperature_9;

bc_data_stream_t *sm_temperature[] =
{
    &sm_temperature_0,
    &sm_temperature_1,
    &sm_temperature_2,
    &sm_temperature_3,
    &sm_temperature_4,
    &sm_temperature_5,
    &sm_temperature_6,
    &sm_temperature_7,
    &sm_temperature_8,
    &sm_temperature_9
};

BC_DATA_STREAM_FLOAT_BUFFER(sm_voltage_buffer, 8)

bc_data_stream_t sm_voltage;

// LED instance
bc_led_t led;
// Button instance
bc_button_t button;
// Lora instance
bc_cmwx1zzabz_t lora;
// ds18b20 library instance
static bc_ds18b20_t ds18b20;
// ds18b20 sensors array
static bc_ds18b20_sensor_t ds18b20_sensors[DS18B20_SENSOR_COUNT];

bc_scheduler_task_id_t battery_measure_task_id;

enum {
    HEADER_BOOT         = 0x00,
    HEADER_UPDATE       = 0x01,
    HEADER_BUTTON_CLICK = 0x02,
    HEADER_BUTTON_HOLD  = 0x03,

} header = HEADER_BOOT;


void handler_battery(bc_module_battery_event_t e, void *p);

void handler_ds18b20(bc_ds18b20_t *s, uint64_t device_id, bc_ds18b20_event_t e, void *p);

void climate_module_event_handler(bc_module_climate_event_t event, void *event_param);

void switch_to_normal_mode_task(void *param);

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) event_param;

    if (event == BC_BUTTON_EVENT_CLICK)
    {
        header = HEADER_BUTTON_CLICK;

        bc_scheduler_plan_now(0);
    }
    else if (event == BC_BUTTON_EVENT_HOLD)
    {
        header = HEADER_BUTTON_HOLD;

        bc_scheduler_plan_now(0);
    }
}

void battery_event_handler(bc_module_battery_event_t event, void *event_param)
{
    if (event == BC_MODULE_BATTERY_EVENT_UPDATE)
    {
        float voltage = NAN;

        bc_module_battery_get_voltage(&voltage);

        bc_data_stream_feed(&sm_voltage, &voltage);
    }
}

void battery_measure_task(void *param)
{
    if (!bc_module_battery_measure())
    {
        bc_scheduler_plan_current_now();
    }
}

void handler_ds18b20(bc_ds18b20_t *self, uint64_t device_address, bc_ds18b20_event_t event, void *event_param)
{
    (void) event_param;

    float value = NAN;

    if (event == BC_DS18B20_EVENT_UPDATE)
    {
        bc_ds18b20_get_temperature_celsius(self, device_address, &value);
        int device_index = bc_ds18b20_get_index_by_device_address(self, device_address);

        //bc_log_debug("UPDATE %" PRIx64 "(%d) = %f", device_address, device_index, value);

        bc_data_stream_feed(sm_temperature[device_index], &value);
    }
}

void lora_callback(bc_cmwx1zzabz_t *self, bc_cmwx1zzabz_event_t event, void *event_param)
{
    if (event == BC_CMWX1ZZABZ_EVENT_ERROR)
    {
        bc_led_set_mode(&led, BC_LED_MODE_BLINK_FAST);
    }
    else if (event == BC_CMWX1ZZABZ_EVENT_SEND_MESSAGE_START)
    {
        bc_led_set_mode(&led, BC_LED_MODE_ON);

        bc_scheduler_plan_relative(battery_measure_task_id, 20);
    }
    else if (event == BC_CMWX1ZZABZ_EVENT_SEND_MESSAGE_DONE)
    {
        bc_led_set_mode(&led, BC_LED_MODE_OFF);
    }
    else if (event == BC_CMWX1ZZABZ_EVENT_READY)
    {
        bc_led_set_mode(&led, BC_LED_MODE_OFF);
    }
    else if (event == BC_CMWX1ZZABZ_EVENT_JOIN_SUCCESS)
    {
        bc_atci_printf("$JOIN_OK");
    }
    else if (event == BC_CMWX1ZZABZ_EVENT_JOIN_ERROR)
    {
        bc_atci_printf("$JOIN_ERROR");
    }
}

bool at_send(void)
{
    bc_scheduler_plan_now(0);

    return true;
}


bool at_status(void)
{
    float value_avg = NAN;

    if (bc_data_stream_get_average(&sm_voltage, &value_avg))
    {
        bc_atci_printf("$STATUS: \"Voltage\",%.1f", value_avg);
    }
    else
    {
        bc_atci_printf("$STATUS: \"Voltage\",");
    }

    int sensor_found = bc_ds18b20_get_sensor_found(&ds18b20);

    for (int i = 0; i < sensor_found; i++)
    {
        value_avg = NAN;

        if (bc_data_stream_get_average(sm_temperature[i], &value_avg))
        {
            bc_atci_printf("$STATUS: \"Temperature%d\",%.1f", i, value_avg);
        }
        else
        {
            bc_atci_printf("$STATUS: \"Temperature%d\",", i);
        }
    }

    return true;
}

void application_init(void)
{
    // bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_OFF);

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize battery
    bc_module_battery_init();
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    battery_measure_task_id = bc_scheduler_register(battery_measure_task, NULL, 2020);

    // Initialize 1-Wire temperature sensors
    bc_ds18b20_init_multiple(&ds18b20, ds18b20_sensors, DS18B20_SENSOR_COUNT, BC_DS18B20_RESOLUTION_BITS_12);
    bc_ds18b20_set_event_handler(&ds18b20, handler_ds18b20, NULL);
    bc_ds18b20_set_update_interval(&ds18b20, MEASURE_INTERVAL);

    // Init stream buffers for averaging
    bc_data_stream_init(&sm_voltage, 1, &sm_voltage_buffer);
    bc_data_stream_init(&sm_temperature_0, 1, &sm_temperature_buffer_0);
    bc_data_stream_init(&sm_temperature_1, 1, &sm_temperature_buffer_1);
    bc_data_stream_init(&sm_temperature_2, 1, &sm_temperature_buffer_2);
    bc_data_stream_init(&sm_temperature_3, 1, &sm_temperature_buffer_3);
    bc_data_stream_init(&sm_temperature_4, 1, &sm_temperature_buffer_4);
    bc_data_stream_init(&sm_temperature_5, 1, &sm_temperature_buffer_5);
    bc_data_stream_init(&sm_temperature_6, 1, &sm_temperature_buffer_6);
    bc_data_stream_init(&sm_temperature_7, 1, &sm_temperature_buffer_7);
    bc_data_stream_init(&sm_temperature_8, 1, &sm_temperature_buffer_8);
    bc_data_stream_init(&sm_temperature_9, 1, &sm_temperature_buffer_9);

    // Initialize lora module
    bc_cmwx1zzabz_init(&lora, BC_UART_UART1);
    bc_cmwx1zzabz_set_event_handler(&lora, lora_callback, NULL);
    bc_cmwx1zzabz_set_mode(&lora, BC_CMWX1ZZABZ_CONFIG_MODE_ABP);
    bc_cmwx1zzabz_set_class(&lora, BC_CMWX1ZZABZ_CONFIG_CLASS_A);

    // Initialize AT command interface
    at_init(&led, &lora);
    static const bc_atci_command_t commands[] = {
            AT_LORA_COMMANDS,
            {"$SEND", at_send, NULL, NULL, NULL, "Immediately send packet"},
            {"$STATUS", at_status, NULL, NULL, NULL, "Show status"},
            AT_LED_COMMANDS,
            BC_ATCI_COMMAND_CLAC,
            BC_ATCI_COMMAND_HELP
    };
    bc_atci_init(commands, BC_ATCI_COMMANDS_LENGTH(commands));

    // Plan task 0 (application_task) to be run after 10 seconds
    bc_scheduler_plan_relative(0, 10 * 1000);
    
    bc_led_pulse(&led, 2000);

    //bc_log_init(BC_LOG_LEVEL_DEBUG, BC_LOG_TIMESTAMP_ABS);
}


void application_task(void)
{
    if (!bc_cmwx1zzabz_is_ready(&lora))
    {
        bc_scheduler_plan_current_relative(100);

        return;
    }

    static uint8_t buffer[1 + 1 + 2 * (DS18B20_SENSOR_COUNT)];
    size_t len = 0;

    memset(buffer, 0xff, sizeof(buffer));

    buffer[len++] = header;

    float voltage_avg = NAN;

    bc_data_stream_get_average(&sm_voltage, &voltage_avg);

    buffer[len++] = !isnan(voltage_avg) ? ceil(voltage_avg * 10.f) : 0xff;   

    int sensor_found = bc_ds18b20_get_sensor_found(&ds18b20);

    for (int i = 0; i < sensor_found; i++)
    {
        float temperature_avg = NAN;

        bc_data_stream_get_average(sm_temperature[i], &temperature_avg);

        if (!isnan(temperature_avg))
        {
            int16_t temperature_i16 = (int16_t) (temperature_avg * 10.f);

            buffer[len++] = temperature_i16 >> 8;
            buffer[len++] = temperature_i16;
        }
    }

    bc_cmwx1zzabz_send_message(&lora, buffer, len);

    static char tmp[sizeof(buffer) * 2 + 1];

    for (size_t i = 0; i < len; i++)
    {
        sprintf(tmp + i * 2, "%02x", buffer[i]);
    }

    bc_atci_printf("$SEND: %s", tmp);

    header = HEADER_UPDATE;

    bc_scheduler_plan_current_relative(SEND_DATA_INTERVAL);
}
