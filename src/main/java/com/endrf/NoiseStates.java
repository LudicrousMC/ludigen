package com.endrf;

import com.endrf.mixin.BlendedNoiseAccessor;
import com.endrf.mixin.ImprovedNoiseAccessor;
import com.endrf.mixin.NormalNoiseAccessor;
import com.endrf.mixin.PerlinNoiseAccessor;
import it.unimi.dsi.fastutil.doubles.DoubleList;
import net.minecraft.world.level.levelgen.DensityFunction;
import net.minecraft.world.level.levelgen.LegacyRandomSource;
import net.minecraft.world.level.levelgen.PositionalRandomFactory;
import net.minecraft.world.level.levelgen.XoroshiroRandomSource;
import net.minecraft.world.level.levelgen.synth.BlendedNoise;
import net.minecraft.world.level.levelgen.synth.ImprovedNoise;
import net.minecraft.world.level.levelgen.synth.NormalNoise;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

public class NoiseStates {
    public static HashMap<String, Integer> noiseStatesMap = new HashMap<>();
    public static ArrayList<NormalNoiseState> noiseStates = new ArrayList<>();
    public static ArrayList<ImprovedNoiseState> noiseLevelStates = new ArrayList<>();

    // Returns position in levelNoiseStates
    public static int registerNoiseState(PositionalRandomFactory rand, String path, Map<String, JsonManager.NoiseParameter> noiseParams) {
        String randType = rand instanceof XoroshiroRandomSource.XoroshiroPositionalRandomFactory
                ? "1"
                : rand instanceof LegacyRandomSource.LegacyPositionalRandomFactory
                ? "0" : "-1";
        String noiseStatePath = path + randType;

        if (noiseStatesMap.containsKey(noiseStatePath)) {
            return noiseStatesMap.get(noiseStatePath);
        }
        JsonManager.NoiseParameter param = noiseParams.get(path);
        //System.out.println("noise param " + noiseStatePath + " " + param.firstOctave);
        NormalNoise noise = NormalNoise.create(rand.fromHashOf(path), new NormalNoise.NoiseParameters(param.firstOctave, DoubleList.of(param.amplitudes)));
        noiseStatesMap.put(noiseStatePath, noiseStates.size());
        noiseStates.add(createNoiseState(noise, param));
        return noiseStates.size() - 1;
    }

    private static NormalNoiseState createNoiseState(NormalNoise noise, JsonManager.NoiseParameter params) {
        NormalNoiseAccessor noiseAccessor = (NormalNoiseAccessor)noise;
        PerlinNoiseAccessor perlin1 = (PerlinNoiseAccessor)noiseAccessor.getFirst();
        ImprovedNoise[] perlin1Levels = perlin1.getNoiseLevels();
        PerlinNoiseState perlinState1 = new PerlinNoiseState(
                perlin1.getLowestFreqValueFactor(),
                perlin1.getLowestFreqInputFactor(),
                perlin1Levels.length,
                noiseLevelStates.size()
        );
        for (int i = 0; i < params.amplitudes.length; i++) {
            ImprovedNoise iNoise = perlin1Levels[i];
            if (iNoise == null) {
                noiseLevelStates.add(ImprovedNoiseState.createDisabled());
            } else {
                noiseLevelStates.add(new ImprovedNoiseState(
                        iNoise.xo,
                        iNoise.yo,
                        iNoise.zo,
                        ((ImprovedNoiseAccessor) (Object) iNoise).getP(),
                        params.amplitudes[i],
                        false
                ));
            }
        }
        PerlinNoiseAccessor perlin2 = (PerlinNoiseAccessor)noiseAccessor.getSecond();
        ImprovedNoise[] perlin2Levels = perlin2.getNoiseLevels();
        PerlinNoiseState perlinState2 = new PerlinNoiseState(
                perlin2.getLowestFreqValueFactor(),
                perlin2.getLowestFreqInputFactor(),
                perlin2Levels.length,
                noiseLevelStates.size()
        );
        for (int i = 0; i < params.amplitudes.length; i++) {
            ImprovedNoise iNoise = perlin2Levels[i];
            if (iNoise == null) {
                noiseLevelStates.add(ImprovedNoiseState.createDisabled());
            } else {
                noiseLevelStates.add(new ImprovedNoiseState(
                        iNoise.xo,
                        iNoise.yo,
                        iNoise.zo,
                        ((ImprovedNoiseAccessor) (Object) iNoise).getP(),
                        params.amplitudes[i],
                        false
                ));
            }
        }
        return new NormalNoiseState(
                new PerlinNoiseState[]{perlinState1, perlinState2},
                noiseAccessor.getMaxValue(),
                noiseAccessor.getValueFactor()
        );
    }

    public static void addOldBlendedNoise() {
        BlendedNoiseAccessor blendedNoise = (BlendedNoiseAccessor)BlendedNoise.createUnseeded(0.0, 0.0, 0.0, 0.0, 0.0);
        PerlinNoiseAccessor perlin1 = (PerlinNoiseAccessor)blendedNoise.getMaxLimitNoise();
        ImprovedNoise[] perlin1Levels = perlin1.getNoiseLevels();
        PerlinNoiseState perlinState1 = new PerlinNoiseState(
                perlin1.getLowestFreqValueFactor(),
                perlin1.getLowestFreqInputFactor(),
                perlin1Levels.length,
                noiseLevelStates.size()
        );
        for (int i = 0; i < 16; i++) {
            ImprovedNoise iNoise = perlin1Levels[i];
            if (iNoise == null) {
                noiseLevelStates.add(ImprovedNoiseState.createDisabled());
            } else {
                noiseLevelStates.add(new ImprovedNoiseState(
                        iNoise.xo,
                        iNoise.yo,
                        iNoise.zo,
                        ((ImprovedNoiseAccessor) (Object) iNoise).getP(),
                        1.0,
                        false
                ));
            }
        }
        PerlinNoiseAccessor perlin2 = (PerlinNoiseAccessor)blendedNoise.getMinLimitNoise();
        ImprovedNoise[] perlin2Levels = perlin2.getNoiseLevels();
        for (int i = 0; i < 16; i++) {
            ImprovedNoise iNoise = perlin2Levels[i];
            if (iNoise == null) {
                noiseLevelStates.add(ImprovedNoiseState.createDisabled());
            } else {
                noiseLevelStates.add(new ImprovedNoiseState(
                        iNoise.xo,
                        iNoise.yo,
                        iNoise.zo,
                        ((ImprovedNoiseAccessor) (Object) iNoise).getP(),
                        1.0,
                        false
                ));
            }
        }
        PerlinNoiseAccessor perlin3 = (PerlinNoiseAccessor)blendedNoise.getMainNoise();
        ImprovedNoise[] perlin3Levels = perlin3.getNoiseLevels();
        PerlinNoiseState perlinState3 = new PerlinNoiseState(
                perlin3.getLowestFreqValueFactor(),
                perlin3.getLowestFreqInputFactor(),
                perlin3Levels.length,
                noiseLevelStates.size()
        );
        for (int i = 0; i < 8; i++) {
            ImprovedNoise iNoise = perlin3Levels[i];
            if (iNoise == null) {
                noiseLevelStates.add(ImprovedNoiseState.createDisabled());
            } else {
                noiseLevelStates.add(new ImprovedNoiseState(
                        iNoise.xo,
                        iNoise.yo,
                        iNoise.zo,
                        ((ImprovedNoiseAccessor) (Object) iNoise).getP(),
                        1.0,
                        false
                ));
            }
        }
        noiseStates.add(new NormalNoiseState(
                new PerlinNoiseState[]{perlinState1, perlinState3},
                blendedNoise.getMaxValue(),
                0.0
        ));
    }

    public static class NormalNoiseState {
        public PerlinNoiseState[] noises;
        public double maxValue;
        public double valueFactor;

        public NormalNoiseState(PerlinNoiseState[] noises, double maxValue, double valueFactor) {
            this.noises = noises;
            this.maxValue = maxValue;
            this.valueFactor = valueFactor;
        }
    }

    public static class PerlinNoiseState {
        public double lowestFreqValueFactor;
        public double lowestFreqInputFactor;
        public int levelsCount;
        public int levelsIndex;

        public PerlinNoiseState(double lowestFreqValueFactor, double lowestFreqInputFactor, int levelsCount, int levelsIndex) {
            this.lowestFreqValueFactor = lowestFreqValueFactor;
            this.lowestFreqInputFactor = lowestFreqInputFactor;
            this.levelsCount = levelsCount;
            this.levelsIndex = levelsIndex;
        }
    }

    public static class ImprovedNoiseState {
        public double x;
        public double y;
        public double z;
        public byte[] values;
        public double amplitude;
        public boolean disabled;

        public ImprovedNoiseState(double x, double y, double z, byte[] values, double amplitude, boolean disabled) {
            this.x = x;
            this.y = y;
            this.z = z;
            this.values = values;
            this.amplitude = amplitude;
            this.disabled = disabled;
        }

        public static ImprovedNoiseState createDisabled() {
            return new ImprovedNoiseState(0, 0, 0, null, 0, true);
        }
    }
}
