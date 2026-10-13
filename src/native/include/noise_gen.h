#ifndef NOISE_GEN_H
#define NOISE_GEN_H

#ifndef SHADER
#include <stdint.h>
#include <math.h>
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
    uint8_t values[512]; // 256 duplicated values for modulus reduction (& 0xFF/255)
    uint8_t disabled;
};

#ifndef SHADER
struct ImprovedNoiseWrapper {
    ImprovedNoise* noise_levels;
    int index;
};
#endif

struct PerlinNoise {
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

#ifndef SHADER
double generate_improved_noise(
    ImprovedNoise* noise,
    double x,
    double y,
    double z,
    double val1,
    double val2
);

ImprovedNoise* get_perlin_noise_level(PerlinNoise* noise, ImprovedNoise* noise_levels, int level);

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

static inline double wrap(double val) {
    return val - floor((val / (double)(1 << 25)) + 0.5) * (double)(1 << 25);
}
#endif

// Predefined Noise states for GPU compute shader
#ifdef SHADER
// values obtained from Minecraft SimplexNoise generator
const ImprovedNoise SIMPLEX_END_ISLANDS = {
    0.0, // x
    0.0, // y
    0.0, // z
    0.0, // amplitude
    { // values
        uint8_t(9), uint8_t(165), uint8_t(129), uint8_t(139), uint8_t(252), uint8_t(242), uint8_t(221), uint8_t(76), uint8_t(188), uint8_t(103), uint8_t(25), uint8_t(205), uint8_t(168), uint8_t(148), uint8_t(130), uint8_t(254),
        uint8_t(104), uint8_t(79), uint8_t(215), uint8_t(189), uint8_t(178), uint8_t(88), uint8_t(147), uint8_t(245), uint8_t(138), uint8_t(232), uint8_t(95), uint8_t(135), uint8_t(214), uint8_t(166), uint8_t(13), uint8_t(160),
        uint8_t(247), uint8_t(31), uint8_t(53), uint8_t(28), uint8_t(115), uint8_t(213), uint8_t(77), uint8_t(156), uint8_t(142), uint8_t(106), uint8_t(57), uint8_t(112), uint8_t(111), uint8_t(217), uint8_t(236), uint8_t(71),
        uint8_t(44), uint8_t(0), uint8_t(56), uint8_t(105), uint8_t(94), uint8_t(87), uint8_t(102), uint8_t(45), uint8_t(18), uint8_t(85), uint8_t(70), uint8_t(199), uint8_t(231), uint8_t(253), uint8_t(10), uint8_t(52),
        uint8_t(20), uint8_t(184), uint8_t(119), uint8_t(176), uint8_t(92), uint8_t(99), uint8_t(145), uint8_t(193), uint8_t(26), uint8_t(6), uint8_t(161), uint8_t(203), uint8_t(198), uint8_t(113), uint8_t(174), uint8_t(108),
        uint8_t(48), uint8_t(173), uint8_t(63), uint8_t(59), uint8_t(227), uint8_t(81), uint8_t(140), uint8_t(40), uint8_t(134), uint8_t(137), uint8_t(109), uint8_t(21), uint8_t(150), uint8_t(196), uint8_t(164), uint8_t(143),
        uint8_t(152), uint8_t(54), uint8_t(29), uint8_t(228), uint8_t(41), uint8_t(16), uint8_t(225), uint8_t(131), uint8_t(93), uint8_t(154), uint8_t(229), uint8_t(144), uint8_t(222), uint8_t(237), uint8_t(151), uint8_t(230),
        uint8_t(204), uint8_t(208), uint8_t(61), uint8_t(100), uint8_t(185), uint8_t(218), uint8_t(226), uint8_t(181), uint8_t(123), uint8_t(220), uint8_t(5), uint8_t(191), uint8_t(255), uint8_t(197), uint8_t(34), uint8_t(172),
        uint8_t(82), uint8_t(90), uint8_t(17), uint8_t(238), uint8_t(194), uint8_t(180), uint8_t(86), uint8_t(243), uint8_t(170), uint8_t(234), uint8_t(32), uint8_t(171), uint8_t(15), uint8_t(38), uint8_t(74), uint8_t(66),
        uint8_t(89), uint8_t(122), uint8_t(157), uint8_t(162), uint8_t(209), uint8_t(211), uint8_t(47), uint8_t(153), uint8_t(233), uint8_t(49), uint8_t(30), uint8_t(64), uint8_t(50), uint8_t(110), uint8_t(120), uint8_t(149),
        uint8_t(212), uint8_t(14), uint8_t(7), uint8_t(55), uint8_t(182), uint8_t(133), uint8_t(37), uint8_t(124), uint8_t(117), uint8_t(177), uint8_t(75), uint8_t(250), uint8_t(155), uint8_t(24), uint8_t(121), uint8_t(216),
        uint8_t(114), uint8_t(46), uint8_t(128), uint8_t(210), uint8_t(118), uint8_t(201), uint8_t(12), uint8_t(80), uint8_t(183), uint8_t(251), uint8_t(126), uint8_t(96), uint8_t(169), uint8_t(22), uint8_t(84), uint8_t(240),
        uint8_t(3), uint8_t(36), uint8_t(200), uint8_t(42), uint8_t(187), uint8_t(132), uint8_t(73), uint8_t(58), uint8_t(235), uint8_t(97), uint8_t(190), uint8_t(11), uint8_t(107), uint8_t(206), uint8_t(202), uint8_t(163),
        uint8_t(175), uint8_t(248), uint8_t(19), uint8_t(159), uint8_t(62), uint8_t(65), uint8_t(246), uint8_t(60), uint8_t(8), uint8_t(67), uint8_t(223), uint8_t(69), uint8_t(4), uint8_t(192), uint8_t(116), uint8_t(167),
        uint8_t(43), uint8_t(244), uint8_t(91), uint8_t(146), uint8_t(27), uint8_t(83), uint8_t(179), uint8_t(136), uint8_t(158), uint8_t(68), uint8_t(141), uint8_t(239), uint8_t(186), uint8_t(101), uint8_t(125), uint8_t(1),
        uint8_t(127), uint8_t(51), uint8_t(23), uint8_t(195), uint8_t(33), uint8_t(249), uint8_t(39), uint8_t(35), uint8_t(224), uint8_t(72), uint8_t(98), uint8_t(219), uint8_t(241), uint8_t(207), uint8_t(2), uint8_t(78),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0),
        uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0)
    },
    uint8_t(0) // disabled
};

#endif // ifdef SHADER

#endif // header