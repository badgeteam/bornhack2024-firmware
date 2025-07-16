
// SPDX-CopyRightText: 2025 Julian Scheffers
// SPDX-License-Identifer: MIT

#include "flags.h"

// Helper macro for constructing a flag.
#define FLAG_DEF(...)                                                      \
    ((flag_t){                                                             \
        .bands_len = sizeof((rgb_t const[]){__VA_ARGS__}) / sizeof(rgb_t), \
        .bands     = (rgb_t const[]){__VA_ARGS__},                         \
    })

// Array of all flags.
// See a missing one? Make a pull request ;)
flag_t const flags[] = {
    // clang-format off
    FLAG_DEF( // Rainbow flag.
        {230,   0,   0},
        {255, 142,   0},
        {255, 239,   0},
        {  0, 130,  27},
        {  0,  75, 255},
        {120,   0, 137},
    ),
    FLAG_DEF( // Trans flag.
        { 71, 206, 250},
        {243, 148, 163},
        {255, 255, 255},
        {243, 148, 163},
        { 71, 206, 250},
    ),
    FLAG_DEF( // Non-binary flag.
        {255, 244,  47},
        {255, 255, 255},
        {156,  89, 209},
        { 41,  41,  41},
    ),
    FLAG_DEF( // Aromantic flag.
        { 58, 167,  64},
        {168, 212, 122},
        {255, 255, 255},
        {151, 151, 151},
        { 41,  41,  41},
    ),
    FLAG_DEF( // Asexual flag.
        { 41,  41,  41},
        {151, 151, 151},
        {255, 255, 255},
        {129,   0, 129},
    ),
    FLAG_DEF( // Bisexual flag.
        {215,   0, 133},
        {215,   0, 133},
        {156,  78, 151},
        {  0,  53, 170},
        {  0,  53, 170},
    ),
    FLAG_DEF( // Pansexual flag.
        {255,  27, 141},
        {255, 217,   0},
        { 27, 179, 255},
    ),
    // clang-format on
};

// Number of flags.
size_t const flags_len = sizeof(flags) / sizeof(flag_t);
