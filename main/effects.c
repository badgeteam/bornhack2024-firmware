
// SPDX-CopyRightText: 2025 Julian Scheffers
// SPDX-License-Identifer: MIT

#include "effects.h"
#include <math.h>
#include "bsp/led.h"

#define LED_COUNT 16

// Brightness multiplier.
float brightness = 1;

// A uint8_t red, green, blue tuple.
typedef struct {
    uint8_t r, g, b;
} rgb_t;

// Convert float HSV into uint8_t RGB.
static rgb_t f_hsv_to_rgb(float h, float s, float v) {
    h = fmodf(h, 1);
    float r, g, b;

    int   i = (int)(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    // clang-format off
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r = g = b = 0.0f; break; // unreachable
    }
    // clang-format on

    rgb_t rgb;
    rgb.r = (uint8_t)(r * 255.0f);
    rgb.g = (uint8_t)(g * 255.0f);
    rgb.b = (uint8_t)(b * 255.0f);
    return rgb;
}

// Convert float RGB into uint8_t RGB.
static rgb_t f_rgb(float r, float g, float b) {
    rgb_t rgb;
    rgb.r = (uint8_t)(r * 255.0f);
    rgb.g = (uint8_t)(g * 255.0f);
    rgb.b = (uint8_t)(b * 255.0f);
    return rgb;
}

// A static buffer to put LED data into.
static uint8_t led_data[3 * LED_COUNT];

// Set the LED data for a particular LED.
static inline void set_led(size_t index, rgb_t col) {
    led_data[index * 3 + 0] = col.r * brightness;
    led_data[index * 3 + 1] = col.g * brightness;
    led_data[index * 3 + 2] = col.b * brightness;
}

// Update the LEDs.
static void update_leds() {
    bsp_led_write(led_data, sizeof(led_data));
}

// A simple hue spectrum effect.
static void effect_hue_spectrum(float coeff) {
    for (size_t i = 0; i < LED_COUNT; i++) {
        set_led(i, f_hsv_to_rgb(coeff + i / (float)LED_COUNT, 1, 1));
    }
    update_leds();
}

// A uniform hue shift.
static void effect_hue_single(float coeff) {
    rgb_t col = f_hsv_to_rgb(coeff, 1, 1);
    for (size_t i = 0; i < LED_COUNT; i++) {
        set_led(i, col);
    }
    update_leds();
}

// A knight rider like effect.
static void effect_knight_rider(float coeff) {
    coeff     = fmodf(coeff, 1);
    float pos = coeff < 0.5 ? coeff * 2 : 2 - coeff * 2;
    for (size_t i = 0; i < LED_COUNT; i++) {
        float dist = pos - i / (float)(LED_COUNT - 1);
        float a    = fmaxf(0, 1 - 4.5 * dist * dist);
        set_led(i, f_rgb(0, a * 0.8, a));
    }
    update_leds();
}

// Table of all effects.
effect_t const effects[] = {
    effect_hue_spectrum,
    effect_hue_single,
    effect_knight_rider,
};

// Number of effects.
size_t const effects_len = sizeof(effects) / sizeof(effect_t);
