package com.endrf.mixin;

import net.minecraft.world.level.levelgen.synth.NormalNoise;
import net.minecraft.world.level.levelgen.synth.PerlinNoise;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

@Mixin(NormalNoise.class)
public interface NormalNoiseAccessor {
    @Accessor("first")
    PerlinNoise getFirst();

    @Accessor("second")
    PerlinNoise getSecond();

    @Accessor("valueFactor")
    double getValueFactor();

    @Accessor("maxValue")
    double getMaxValue();
}
