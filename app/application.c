#include <application.h>
#include <radio.h>
#include <bc_thermistor.h>

#define UPDATE_INTERVAL 10

static void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param);

void application_init(void)
{
    static bc_led_t led;

    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_OFF);

    static bc_button_t button;

    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, &led);

    bc_module_battery_init(BC_MODULE_BATTERY_FORMAT_MINI);

    bc_thermistor_init();

    bc_radio_init();

    // Plan application_task
    bc_scheduler_plan_relative(0, UPDATE_INTERVAL * 1000);
}

// TODO Replace with thermistor_event_handler once bc_thermistor is implemented
void application_task(void)
{
    float temperature;

    // If measured temperature is out of range...
    if (!bc_thermistor_get_temperature_celsius(&temperature))
    {
        temperature = NAN;
    }

    bc_radio_pub_thermometer(RADIO_CHANNEL_A, &temperature);

    bc_scheduler_plan_current_relative(UPDATE_INTERVAL * 1000);
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
