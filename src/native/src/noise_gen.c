#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "noise_gen.h"
#include "util.h"

typedef struct {
    int x, y, z;
} Gradient;

const Gradient SIMPLEX_GRADIENT[16] = {
    { 1,  1,  0},
    {-1,  1,  0},
    { 1, -1,  0},
    {-1, -1,  0},
    { 1,  0,  1},
    {-1,  0,  1},
    { 1,  0, -1},
    {-1,  0, -1},
    { 0,  1,  1},
    { 0, -1,  1},
    { 0,  1, -1},
    { 0, -1, -1},
    { 1,  1,  0},
    { 0, -1,  1},
    {-1,  1,  0},
    { 0, -1, -1}
};

static inline double sample_plus_lerp(
    uint8_t* noise,
    int x,
    int y,
    int z,
    double x_real,
    double y_real,
    double z_real,
    double y_real_offset
) {
    int x0 = noise[x];
    int x1 = noise[x + 1];
    int xy00 = noise[x0 + y];
    int xy01 = noise[x0 + y + 1];
    int xy10 = noise[x1 + y];
    int xy11 = noise[x1 + y + 1];

    int xyz000 = noise[xy00 + z] & 0xF;
    int xyz100 = noise[xy10 + z] & 0xF;
    int xyz010 = noise[xy01 + z] & 0xF;
    int xyz110 = noise[xy11 + z] & 0xF;
    int xyz001 = noise[xy00 + z + 1] & 0xF;
    int xyz101 = noise[xy10 + z + 1] & 0xF;
    int xyz011 = noise[xy01 + z + 1] & 0xF;
    int xyz111 = noise[xy11 + z + 1] & 0xF;

    double x_real1 = x_real - 1.0;
    double y_real1 = y_real - 1.0;
    double z_real1 = z_real - 1.0;
    Gradient s000 = SIMPLEX_GRADIENT[xyz000];
    double d000 = (double)s000.x * x_real
                + (double)s000.y * y_real
                + (double)s000.z * z_real;
    Gradient s100 = SIMPLEX_GRADIENT[xyz100];
    double d100 = (double)s100.x * x_real1
                + (double)s100.y * y_real
                + (double)s100.z * z_real;
    Gradient s010 = SIMPLEX_GRADIENT[xyz010];
    double d010 = (double)s010.x * x_real
                + (double)s010.y * y_real1
                + (double)s010.z * z_real;
    Gradient s110 = SIMPLEX_GRADIENT[xyz110];
    double d110 = (double)s110.x * x_real1
                + (double)s110.y * y_real1
                + (double)s110.z * z_real;
    Gradient s001 = SIMPLEX_GRADIENT[xyz001];
    double d001 = (double)s001.x * x_real
                + (double)s001.y * y_real
                + (double)s001.z * z_real1;
    Gradient s101 = SIMPLEX_GRADIENT[xyz101];
    double d101 = (double)s101.x * x_real1
                + (double)s101.y * y_real
                + (double)s101.z * z_real1;
    Gradient s011 = SIMPLEX_GRADIENT[xyz011];
    double d011 = (double)s011.x * x_real
                + (double)s011.y * y_real1
                + (double)s011.z * z_real1;
    Gradient s111 = SIMPLEX_GRADIENT[xyz111];
    double d111 = (double)s111.x * x_real1
                + (double)s111.y * y_real1
                + (double)s111.z * z_real1;

    return lerp3(
        smoothstep(x_real),
        smoothstep(y_real_offset),
        smoothstep(z_real),
        d000,
        d100,
        d010,
        d110,
        d001,
        d101,
        d011,
        d111
    );
}

double generate_improved_noise(
    ImprovedNoise* noise,
    double x,
    double y,
    double z,
    double val1,
    double val2
) {
    x += noise->x;
    y += noise->y;
    z += noise->z;
    int x_floor = (int)floor(x);
    int y_floor = (int)floor(y);
    int z_floor = (int)floor(z);
    x -= x_floor;
    y -= y_floor;
    z -= z_floor;
    double y_offset;
    if (val1 != 0.0) {
        double val = val2 >= 0.0 && val2 < y ? val2 : y;
        y_offset = y - (val1 * floor((val / val1) + 1.0e-7));
    } else {
        y_offset = y;
    }
    return sample_plus_lerp(
        noise->values,
        x_floor & 0xFF,
        y_floor & 0xFF,
        z_floor & 0xFF,
        x, y, z, y_offset
    );
}

static inline double wrap(double val) {
    return val - floor((val / (double)(1 << 25)) + 0.5) * (double)(1 << 25);
}

double generate_perlin_noise(
    PerlinNoise noise,
    ImprovedNoise* noise_levels,
    double x,
    double y,
    double z
) {
    double value = 0.0;
    double input_factor = noise.lowest_freq_input_factor;
    double value_factor = noise.lowest_freq_value_factor;
    for (int i = 0; i < noise.levels_count; i++) {
        ImprovedNoise* level = noise_levels + noise.levels_index + i;
        if (level->disabled == 0) {
            value += level->amplitude
                * generate_improved_noise(
                    level,
                    wrap(x * input_factor),
                    wrap(y * input_factor),
                    wrap(z * input_factor),
                    0.0, 0.0
                ) * value_factor;
        }
        input_factor *= 2.0;
        value_factor /= 2.0;
    }
    return value;
}

double generate_normal_noise(
    NormalNoise* noise,
    ImprovedNoise* noise_levels,
    double x,
    double y,
    double z
) {
    return (
        generate_perlin_noise(noise->noises[0], noise_levels, x, y, z)
        + generate_perlin_noise(
            noise->noises[1],
            noise_levels,
            x * 1.0181268882175227,
            y * 1.0181268882175227,
            z * 1.0181268882175227
        )
    ) * noise->value_factor;
}