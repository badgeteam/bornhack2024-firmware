
// SPDX-CopyRightText: 2025 Julian Scheffers
// SPDX-License-Identifer: MIT

#include "effects.h"
#include <esp_log.h>
#include <math.h>
#include <string.h>
#include "bsp/led.h"
#include "color.h"
#include "flags.h"

#define LED_COUNT 16

// Brightness multiplier.
float brightness = 1;

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

// Helper function for `project_flag` that projects a single color band.
static void project_band(float start, float size, rgb_t col) {
    // Normalize into pixel amounts.
    start     *= LED_COUNT;
    size      *= LED_COUNT;
    float end  = start + size;

    start = fmaxf(0, start);
    end   = fminf(LED_COUNT, end);

    if (start >= end) {
        return;
    }

    int led0 = floorf(start);
    int led1 = ceilf(end);

    for (int led = led0; led <= led1; led++) {
        float cov = fminf(end, led + 1) - fmaxf(start, led);
        if (cov > 0) {
            led_data[led * 3 + 0] += col.r * cov;
            led_data[led * 3 + 1] += col.g * cov;
            led_data[led * 3 + 2] += col.b * cov;
        }
    }
}

// Helper function for `effect_flags` that projects one flag onto (a portion of) the LEDs.
static void project_flag(flag_t flag, float offset) {
    float const band_size = 1.0f / flag.bands_len;
    for (size_t i = 0; i < flag.bands_len; i++) {
        project_band(offset + i * band_size, band_size, flag.bands[i]);
    }
}

// An effect that scrolls through pride flags.
static void effect_flags(float coeff) {
    memset(led_data, 0, sizeof(led_data));
    int flag0 = ((int)coeff) % flags_len;
    int flag1 = (flag0 + 1) % flags_len;
    coeff     = fminf(0, 3 - 4 * fmodf(coeff, 1));
    project_flag(flags[flag0], coeff);
    if (coeff) {
        project_flag(flags[flag1], coeff + 1);
    }
    for (size_t i = 0; i < sizeof(led_data); i++) {
        led_data[i] = led_data[i] * brightness;
    }
    update_leds();
}

// Table of all effects.
effect_t const effects[] = {
    effect_hue_spectrum,
    effect_hue_single,
    effect_knight_rider,
    effect_flags,
};

// Number of effects.
size_t const effects_len = sizeof(effects) / sizeof(effect_t);
