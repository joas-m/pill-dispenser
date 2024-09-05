#ifndef LED_H
#define LED_H

/// Right LED
#define LED_0_PIN 22

/// Middle LED
#define LED_1_PIN 21

/// Left LED
#define LED_2_PIN 20

typedef enum {
    LED_0,
    LED_1,
    LED_2,
} led_t;

/// Initializes board LEDs
void init_leds(void);

/// Set LED state
void set_led_state(led_t led, bool state);

/// Loggles a LED on or off
void toggle_led_state(led_t led);

#endif