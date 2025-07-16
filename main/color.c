
// SPDX-CopyRightText: 2025 Julian Scheffers
// SPDX-License-Identifer: MIT

#include "color.h"

// Convert float HSV into uint8_t RGB.
rgb_t f_hsv_to_rgb(float h, float s, float v) {
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