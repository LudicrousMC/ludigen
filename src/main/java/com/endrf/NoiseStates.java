package com.endrf;

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
                ? "0" : "";
        String noiseStatePath = path + randType;
        if (noiseStatesMap.containsKey(noiseStatePath)) {
            return noiseStatesMap.get(noiseStatePath);
        }
        JsonManager.NoiseParameter param = noiseParams.get(path);
        //System.out.println("noise param " + noiseStatePath + " " + param.firstOctave);
        NormalNoise noise = NormalNoise.create(rand.fromHashOf(path), new NormalNoise.NoiseParameters(param.firstOctave, DoubleList.of(param.amplitudes)));
        noiseStatesMap.put(noiseStatePath, noiseStates.size());
        noiseStates.add(createNoiseState(noise, param));
        if (path.equals("minecraft:ridge")) {
            double result = 0;
            System.out.println("Coordinate:\n\tX | 57392152914566.49321\n\tY | 67432010703250.432\n\tZ | -97419932495.33332");
            long startTime = System.nanoTime();
            result = noise.getValue(57392152914566.49321, 67432010703250.432, -97419932495.33332);
            long endTime = System.nanoTime();
            System.out.println("normalnoise java: " + result);
            System.out.println("time: " + (endTime - startTime) + " nanoseconds");
        }
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
        BlendedNoise oldBlendedNoise = BlendedNoise.createUnseeded(0.25, 0.125, 80.0, 160.0, 8.0);
        System.out.println("old blended " + oldBlendedNoise.compute(new DensityFunction.FunctionContext() {
            @Override
            public int blockX() {
                return 0;
            }

            @Override
            public int blockY() {
                return 0;
            }

            @Override
            public int blockZ() {
                return 0;
            }
        }));
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
