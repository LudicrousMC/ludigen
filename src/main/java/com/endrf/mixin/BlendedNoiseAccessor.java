package com.endrf.mixin;

import net.minecraft.world.level.levelgen.synth.BlendedNoise;
import net.minecraft.world.level.levelgen.synth.PerlinNoise;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

@Mixin(BlendedNoise.class)
public interface BlendedNoiseAccessor {
    @Accessor("maxLimitNoise")
    PerlinNoise getMaxLimitNoise();

    @Accessor("minLimitNoise")
    PerlinNoise getMinLimitNoise();

    @Accessor("mainNoise")
    PerlinNoise getMainNoise();

    @Accessor("maxValue")
    double getMaxValue();
}
