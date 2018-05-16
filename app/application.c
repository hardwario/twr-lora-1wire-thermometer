#include <application.h>
#include <bc_ds18b20.h>

// Time after the sending is less frequent to save battery
#define SERVICE_INTERVAL_INTERVAL (60 * 60 * 1000)
#define BATTERY_UPDATE_INTERVAL   (10 * 60 * 1000)

#define UPDATE_SERVICE_INTERVAL            (5 * 1000)
#define UPDATE_NORMAL_INTERVAL             (10 * 1000)
#define BAROMETER_UPDATE_SERVICE_INTERVAL  (1 * 60 * 1000)
#define BAROMETER_UPDATE_NORMAL_INTERVAL   (5 * 60 * 1000)

#define TEMPERATURE_TAG_PUB_NO_CHANGE_INTEVAL (5 * 60 * 1000)
#define TEMPERATURE_TAG_PUB_VALUE_CHANGE 0.2f

#define HUMIDITY_TAG_PUB_NO_CHANGE_INTEVAL (5 * 60 * 1000)
#define HUMIDITY_TAG_PUB_VALUE_CHANGE 5.0f

#define LUX_METER_TAG_PUB_NO_CHANGE_INTEVAL (5 * 60 * 1000)
#define LUX_METER_TAG_PUB_VALUE_CHANGE 25.0f

#define BAROMETER_TAG_PUB_NO_CHANGE_INTEVAL (5 * 60 * 1000)
#define BAROMETER_TAG_PUB_VALUE_CHANGE 20.0f

static bc_led_t led;
static bc_button_t button;
static bc_ds18b20_t ds18d20;
// Thermometer instance
bc_tmp112_t tmp112;

struct {
    event_param_t temperature;
    event_param_t temperature_ds18b20;
    event_param_t humidity;
    event_param_t illuminance;
    event_param_t pressure;

} params;

void handler_button(bc_button_t *s, bc_button_event_t e, void *p);

void handler_battery(bc_module_battery_event_t e, void *p);

void handler_ds18b20(bc_ds18b20_t *s, bc_ds18b20_event_t e, void *p);

void climate_module_event_handler(bc_module_climate_event_t event, void *event_param);

void switch_to_normal_mode_task(void *param);

void application_init(void)
{
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_OFF);

    bc_radio_init(BC_RADIO_MODE_NODE_SLEEPING);

    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, handler_button, NULL);

    bc_module_battery_init(BC_MODULE_BATTERY_FORMAT_MINI);
    bc_module_battery_set_event_handler(handler_battery, NULL);
    bc_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    bc_ds18b20_init(&ds18d20, BC_DS18B20_RESOLUTION_BITS_12);
    bc_ds18b20_set_event_handler(&ds18d20, handler_ds18b20, NULL);
    bc_ds18b20_set_update_interval(&ds18d20, UPDATE_SERVICE_INTERVAL);

    // Initialize thermometer sensor on core module
    bc_tmp112_init(&tmp112, BC_I2C_I2C0, 0x49);

    // Initialize climate module
    bc_module_climate_init();
    bc_module_climate_set_event_handler(climate_module_event_handler, NULL);
    bc_module_climate_set_update_interval_thermometer(UPDATE_SERVICE_INTERVAL);
    bc_module_climate_set_update_interval_hygrometer(UPDATE_SERVICE_INTERVAL);
    bc_module_climate_set_update_interval_lux_meter(UPDATE_SERVICE_INTERVAL);
    bc_module_climate_set_update_interval_barometer(BAROMETER_UPDATE_NORMAL_INTERVAL);
    bc_module_climate_measure_all_sensors();

    bc_scheduler_register(switch_to_normal_mode_task, NULL, SERVICE_INTERVAL_INTERVAL);

    bc_radio_pairing_request("kit-wireless-pool-sensor", VERSION);

    bc_led_pulse(&led, 2000);
}

void handler_button(bc_button_t *s, bc_button_event_t e, void *p)
{
    (void) s;
    (void) p;

    if (e == BC_BUTTON_EVENT_PRESS)
    {
        bc_led_set_mode(&led, BC_LED_MODE_OFF);
        bc_led_pulse(&led, 100);

        static uint16_t event_count = 0;

        bc_radio_pub_push_button(&event_count);

        event_count++;
    }
}

void handler_battery(bc_module_battery_event_t e, void *p)
{
    (void) e;
    (void) p;

    float voltage;

    if (bc_module_battery_get_voltage(&voltage))
    {
        bc_radio_pub_battery(&voltage);
    }
}

void handler_ds18b20(bc_ds18b20_t *s, bc_ds18b20_event_t e, void *p)
{
    (void) p;

    float value = NAN;

    if (e == bc_ds18b20_EVENT_UPDATE)
    {
        bc_ds18b20_get_temperature_celsius(s, &value);

        if ((fabs(value - params.temperature_ds18b20.value) >= TEMPERATURE_TAG_PUB_VALUE_CHANGE) || (params.temperature_ds18b20.next_pub < bc_scheduler_get_spin_tick()))
        {
            bc_radio_pub_temperature(BC_RADIO_PUB_CHANNEL_A, &value);
            params.temperature_ds18b20.value = value;
            params.temperature_ds18b20.next_pub = bc_scheduler_get_spin_tick() + TEMPERATURE_TAG_PUB_NO_CHANGE_INTEVAL;
        }
    }
}

void switch_to_normal_mode_task(void *param)
{
    bc_module_climate_set_update_interval_thermometer(UPDATE_NORMAL_INTERVAL);
    bc_module_climate_set_update_interval_hygrometer(UPDATE_NORMAL_INTERVAL);
    bc_module_climate_set_update_interval_lux_meter(UPDATE_NORMAL_INTERVAL);
    bc_module_climate_set_update_interval_barometer(BAROMETER_UPDATE_SERVICE_INTERVAL);

    bc_ds18b20_set_update_interval(&ds18d20, UPDATE_NORMAL_INTERVAL);

    bc_scheduler_unregister(bc_scheduler_get_current_task_id());
}

void climate_module_event_handler(bc_module_climate_event_t event, void *event_param)
{
    (void) event_param;

    float value;

    if (event == BC_MODULE_CLIMATE_EVENT_UPDATE_THERMOMETER)
    {
        if (bc_module_climate_get_temperature_celsius(&value))
        {
            if ((fabs(value - params.temperature.value) >= TEMPERATURE_TAG_PUB_VALUE_CHANGE) || (params.temperature.next_pub < bc_scheduler_get_spin_tick()))
            {
                bc_radio_pub_temperature(BC_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &value);
                params.temperature.value = value;
                params.temperature.next_pub = bc_scheduler_get_spin_tick() + TEMPERATURE_TAG_PUB_NO_CHANGE_INTEVAL;
            }
        }
    }
    else if (event == BC_MODULE_CLIMATE_EVENT_UPDATE_HYGROMETER)
    {
        if (bc_module_climate_get_humidity_percentage(&value))
        {
            if ((fabs(value - params.humidity.value) >= HUMIDITY_TAG_PUB_VALUE_CHANGE) || (params.humidity.next_pub < bc_scheduler_get_spin_tick()))
            {
                bc_radio_pub_humidity(BC_RADIO_PUB_CHANNEL_R3_I2C0_ADDRESS_DEFAULT, &value);
                params.humidity.value = value;
                params.humidity.next_pub = bc_scheduler_get_spin_tick() + HUMIDITY_TAG_PUB_NO_CHANGE_INTEVAL;
            }
        }
    }
    else if (event == BC_MODULE_CLIMATE_EVENT_UPDATE_LUX_METER)
    {
        if (bc_module_climate_get_illuminance_lux(&value))
        {
            if (value < 1)
            {
                value = 0;
            }
            if ((fabs(value - params.illuminance.value) >= LUX_METER_TAG_PUB_VALUE_CHANGE) || (params.illuminance.next_pub < bc_scheduler_get_spin_tick()) ||
                    ((value == 0) && (params.illuminance.value != 0)) || ((value > 1) && (params.illuminance.value == 0)))
            {
                bc_radio_pub_luminosity(BC_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &value);
                params.illuminance.value = value;
                params.illuminance.next_pub = bc_scheduler_get_spin_tick() + LUX_METER_TAG_PUB_NO_CHANGE_INTEVAL;
            }
        }
    }
    else if (event == BC_MODULE_CLIMATE_EVENT_UPDATE_BAROMETER)
    {
        if (bc_module_climate_get_pressure_pascal(&value))
        {
            if ((fabs(value - params.pressure.value) >= BAROMETER_TAG_PUB_VALUE_CHANGE) || (params.pressure.next_pub < bc_scheduler_get_spin_tick()))
            {
                float meter;

                if (!bc_module_climate_get_altitude_meter(&meter))
                {
                    return;
                }

                bc_radio_pub_barometer(BC_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &value, &meter);
                params.pressure.value = value;
                params.pressure.next_pub = bc_scheduler_get_spin_tick() + BAROMETER_TAG_PUB_NO_CHANGE_INTEVAL;
            }
        }
    }
}

