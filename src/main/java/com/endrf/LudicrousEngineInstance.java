package com.endrf;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;

public class LudicrousEngineInstance {
    private final long ptr;
    private final HashMap<String, Integer> fdIndexMap;

    public LudicrousEngineInstance(long ptr) {
        this.ptr = ptr;
        this.fdIndexMap = new HashMap<>();
    }

    public void loadFinalDensityShader(
            String dimension,
            int scale,
            int minY,
            int height,
            int noiseX,
            int noiseY,
            String json,
            ArrayList<NoiseStates.NormalNoiseState> noiseStates,
            ArrayList<NoiseStates.ImprovedNoiseState> noiseLevels
    ) {
        this.fdIndexMap.put(dimension, this.fdIndexMap.size());
        LudicrousEngine.loadFinalDensityShader(this.ptr, scale, minY, height, noiseX, noiseY, json, noiseStates, noiseLevels, dimension);
    }

    public double evalDensityFunction(int x, int y, int z) {
        return LudicrousEngine.evalDensityFunction(this.ptr, x, y, z);
    }

    public double[] fillNoise(int minY, int height, int cellWidth, int cellHeight, int x, int z) {
        return LudicrousEngine.fillNoise(this.ptr, minY, height, cellWidth, cellHeight, x, z);
    }

    public ByteBuffer fillNoiseAccel(String dimension, int x, int z, int[] retType) {
        int fdIndex = this.fdIndexMap.get(dimension);
        return LudicrousEngine.fillNoiseAccel(this.ptr, fdIndex, x, z, retType);
    }

    public void releaseDensities(ByteBuffer buf, int cachedChunkIndex) {
        LudicrousEngine.releaseDensities(this.ptr, buf, cachedChunkIndex);
    }
}
