#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>

/// Left button
#define BTN_0_PIN 9

/// Middle button
#define BTN_1_PIN 8

/// Right button
#define BTN_2_PIN 7

typedef enum {
    BTN_0,
    BTN_1,
    BTN_2,
} btn_t;

/// Initializes board buttons
void init_buttons(void);

/// Checks whether a button is pressed
bool btn_pressed(btn_t btn);

#endif