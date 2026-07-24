package com.endrf.mixin;

import net.minecraft.world.level.levelgen.synth.ImprovedNoise;
import net.minecraft.world.level.levelgen.synth.PerlinNoise;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.gen.Accessor;

@Mixin(PerlinNoise.class)
public interface PerlinNoiseAccessor {
    @Accessor("noiseLevels")
    ImprovedNoise[] getNoiseLevels();

    @Accessor("lowestFreqValueFactor")
    double getLowestFreqValueFactor();

    @Accessor("lowestFreqInputFactor")
    double getLowestFreqInputFactor();
}
