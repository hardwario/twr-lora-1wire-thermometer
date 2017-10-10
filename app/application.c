#include <application.h>
#include <radio.h>
#include <bc_thermistor.h>
#include <SPIRIT_Radio.h>

#define UPDATE_INTERVAL (30 * 1000)
#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000)
#define BATTERY_LOW_TEMPERATURE (0.99f)
#define BATTERY_LOW_INTERVAL (6 * 60 * 60 * 1000)

static bc_led_t led;
static bc_button_t button;
static bc_thermistor_t thermistor;

static int battery_low_cnt = 0;
static bc_tick_t battery_low_next = 0;

static void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param);
static void battery_event_handler(bc_module_battery_event_t event, void *event_param);
static void thermistor_event_handler(bc_thermistor_t *self, bc_thermistor_event_t event, void *event_param);

void application_init(void)
{
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_ON);

    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    bc_module_battery_init(BC_MODULE_BATTERY_FORMAT_MINI);
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    bc_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    bc_thermistor_init(&thermistor, BC_MODULE_SENSOR_CHANNEL_A);
    bc_thermistor_set_event_handler(&thermistor, thermistor_event_handler, NULL);
    bc_thermistor_set_update_interval(&thermistor, UPDATE_INTERVAL);

    bc_radio_init();

    SpiritRadioSetDatarate(115200);
    SpiritRadioSetChannelBW(200E3);
    SpiritRadioSetPALevelMaxIndex(0);
    SpiritRadioSetPALeveldBm(0, 0);

    bc_led_set_mode(&led, BC_LED_MODE_OFF);
}

static void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) self;

    if (event == BC_BUTTON_EVENT_PRESS)
    {
        bc_led_set_mode(&led, BC_LED_MODE_OFF);
        bc_led_pulse(&led, 100);

        static uint16_t event_count = 0;

        bc_radio_pub_push_button(&event_count);

        event_count++;
    }
    if (event == BC_BUTTON_EVENT_HOLD)
    {
        bc_radio_enroll_to_gateway();

        bc_led_set_mode(&led, BC_LED_MODE_OFF);
        bc_led_pulse(&led, 1000);
    }
}

static void battery_event_handler(bc_module_battery_event_t event, void *event_param)
{
    (void) event_param;

    float voltage;

    if (bc_module_battery_get_voltage(&voltage))
    {
        bc_radio_pub_battery(BC_MODULE_BATTERY_FORMAT_MINI, &voltage);
    }

    if ((event = BC_MODULE_BATTERY_EVENT_LEVEL_LOW) || (event == BC_MODULE_BATTERY_EVENT_LEVEL_CRITICAL))
    {
        if (battery_low_next < bc_tick_get())
        {
            battery_low_cnt = 3;
            battery_low_next += BATTERY_LOW_INTERVAL;
        }
    }
}

static void thermistor_event_handler(bc_thermistor_t *self, bc_thermistor_event_t event, void *event_param)
{
    float temperature;

    if (!bc_thermistor_get_temperature_celsius(self, &temperature))
    {
        temperature = NAN;
    }

    if (battery_low_cnt > 0)
    {
        battery_low_cnt--;
        temperature = BATTERY_LOW_TEMPERATURE;
    }

    bc_radio_pub_thermometer(RADIO_CHANNEL_A, &temperature);
}
