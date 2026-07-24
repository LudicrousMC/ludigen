#ifndef NOISE_GEN_H
#define NOISE_GEN_H

#ifndef SHADER
#include <stdint.h>
typedef struct ImprovedNoise ImprovedNoise;
typedef struct ImprovedNoiseWrapper ImprovedNoiseWrapper;
typedef struct PerlinNoise PerlinNoise;
typedef struct NormalNoise NormalNoise;
#endif

struct ImprovedNoise {
    double x;
    double y;
    double z;
    double amplitude;
    uint8_t values[512];
    uint8_t disabled;
};

struct ImprovedNoiseWrapper {
    ImprovedNoise* noise_levels;
    int index;
};

struct __attribute__((packed)) PerlinNoise {
    double lowest_freq_value_factor;
    double lowest_freq_input_factor;
    uint8_t levels_count; // Number of improved noise states
    uint16_t levels_index; // Starting index of improved noise states in global array
};

struct NormalNoise {
    PerlinNoise noises[2];
    double max_value;
    double value_factor;
};

double generate_improved_noise(
    ImprovedNoise* noise,
    double x,
    double y,
    double z,
    double val1,
    double val2
);

double generate_perlin_noise(
    PerlinNoise noise,
    ImprovedNoise* noise_levels,
    double x,
    double y,
    double z
);

double generate_normal_noise(
    NormalNoise* noise,
    ImprovedNoise* noise_levels,
    double x,
    double y,
    double z
);

#endif