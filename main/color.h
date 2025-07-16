
// SPDX-CopyRightText: 2025 Julian Scheffers
// SPDX-License-Identifer: MIT

#pragma once

#include <math.h>
#include <stdint.h>

// A uint8_t red, green, blue tuple.
typedef struct {
    uint8_t r, g, b;
} rgb_t;

// Convert float HSV into uint8_t RGB.
rgb_t f_hsv_to_rgb(float h, float s, float v);

// Convert float RGB into uint8_t RGB.
static inline rgb_t f_rgb(float r, float g, float b) {
    rgb_t rgb;
    rgb.r = (uint8_t)(r * 255.0f);
    rgb.g = (uint8_t)(g * 255.0f);
    rgb.b = (uint8_t)(b * 255.0f);
    return rgb;
}
