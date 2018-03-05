#include <application.h>
#include <bc_ds18b20.h>

#define UPDATE_INTERVAL (30 * 1000)

#define BATTERY_UPDATE_INTERVAL (10 * 60 * 1000)

static bc_led_t led;

static bc_button_t button;

static bc_ds18b20_t ds18d20;

void handler_button(bc_button_t *s, bc_button_event_t e, void *p);

void handler_battery(bc_module_battery_event_t e, void *p);

void handler_ds18b20(bc_ds18b20_t *s, bc_ds18b20_event_t e, void *p);

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
    bc_ds18b20_set_update_interval(&ds18d20, UPDATE_INTERVAL);

    bc_radio_pairing_request("tpca-scissor-lift-node", VERSION);

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

    float temperature = NAN;

    if (e == bc_ds18b20_EVENT_UPDATE)
    {
        bc_ds18b20_get_temperature_celsius(s, &temperature);
    }

    bc_radio_pub_temperature(BC_RADIO_PUB_CHANNEL_A, &temperature);
}
