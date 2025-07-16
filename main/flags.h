
// SPDX-CopyRightText: 2025 Julian Scheffers
// SPDX-License-Identifer: MIT

#pragma once

#include <stddef.h>
#include "color.h"

// A simple flag with horizontal color bands.
typedef struct {
    // Number of bands in the flag.
    size_t       bands_len;
    // Flag's color bands.
    rgb_t const* bands;
} flag_t;

// Array of all flags.
// See a missing one? Make a pull request ;)
extern flag_t const flags[];
// Number of flags.
extern size_t const flags_len;
