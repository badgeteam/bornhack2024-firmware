
// SPDX-CopyRightText: 2025 Julian Scheffers
// SPDX-License-Identifer: MIT

#pragma once

#include <stddef.h>
#include <stdint.h>

// Brightness multiplier.
extern float brightness;

// An effect function.
typedef void (*effect_t)(float coeff);

// Table of all effects.
extern effect_t const effects[];
// Number of effects.
extern size_t const   effects_len;
