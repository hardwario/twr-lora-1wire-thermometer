#include <application.h>
#include <radio.h>
#include <bc_thermistor.h>

#define UPDATE_INTERVAL (5 * 1000)
#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000)

//#define TEMPERATURE_DATA_STREAM_SAMPLES 10
//#define TEMPERATURE_UPDATE_INTERVAL (1 * 1000)

static void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param);
static void battery_event_handler(bc_module_battery_event_t event, void *event_param);
static void thermistor_event_handler(bc_thermistor_t *self, bc_thermistor_event_t event, void *event_param);

void application_init(void)
{
    static bc_led_t led;

    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_ON);

    static bc_button_t button;

    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, &led);

    bc_module_battery_init(BC_MODULE_BATTERY_FORMAT_MINI);
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    bc_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    static bc_thermistor_t thermistor;

    bc_thermistor_init(&thermistor, BC_MODULE_SENSOR_CHANNEL_A);
    bc_thermistor_set_event_handler(&thermistor, thermistor_event_handler, NULL);
    bc_thermistor_set_update_interval(&thermistor, UPDATE_INTERVAL);

    bc_radio_init();

    bc_led_set_mode(&led, BC_LED_MODE_OFF);
}

static void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) self;

    bc_led_t *led = event_param;

    if (event == BC_BUTTON_EVENT_HOLD)
    {
        bc_radio_enroll_to_gateway();

        bc_led_set_mode(led, BC_LED_MODE_OFF);
        bc_led_pulse(led, 1000);
    }
}

static void battery_event_handler(bc_module_battery_event_t event, void *event_param)
{
    (void) event;
    (void) event_param;

    float voltage;

    if (bc_module_battery_get_voltage(&voltage))
    {
        bc_radio_pub_battery(BC_MODULE_BATTERY_FORMAT_MINI, &voltage);
    }
}


static void thermistor_event_handler(bc_thermistor_t *self, bc_thermistor_event_t event, void *event_param)
{
    float temperature;

    if (!bc_thermistor_get_temperature_celsius(self, &temperature))
    {
        temperature = NAN;
    }

    bc_radio_pub_thermometer(RADIO_CHANNEL_A, &temperature);
}
