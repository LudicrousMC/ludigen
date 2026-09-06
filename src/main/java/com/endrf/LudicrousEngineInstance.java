package com.endrf;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;

public class LudicrousEngineInstance {
    private final long ptr;
    private final LudicrousEngine engine;

    public LudicrousEngineInstance(long ptr) {
        this.ptr = ptr;
        this.engine = new LudicrousEngine();
    }

    public void registerShader(String shader) throws IOException {
        byte[] shaderBytes;
        try (InputStream in = LudicrousEngine.class.getResourceAsStream("/compute/" + shader + ".spv")) {
            if (in == null) throw new RuntimeException("Missing Compute Shader: " + shader + ".spv");
            shaderBytes = in.readAllBytes();
        }
        ByteBuffer spirv = ByteBuffer.allocateDirect(shaderBytes.length);
        spirv.order(ByteOrder.nativeOrder());
        spirv.put(shaderBytes);
        spirv.flip();
        engine.loadShader(this.ptr, spirv);
    }

    public void setDensityFunction(String json) {
        engine.setDensityFunction(this.ptr, json);
    }

    public void setNoiseStates(ArrayList<NoiseStates.NormalNoiseState> noiseStates, ArrayList<NoiseStates.ImprovedNoiseState> noiseLevelStates) {
        engine.setNoiseStates(this.ptr, noiseStates, noiseLevelStates);
    }

    public double evalDensityFunction(int x, int y, int z) {
        return engine.evalDensityFunction(this.ptr, x, y, z);
    }

    public double[] fillNoise(int minY, int height, int cellWidth, int cellHeight, int x, int z) {
        return engine.fillNoise(this.ptr, minY, height, cellWidth, cellHeight, x, z);
    }

    public ByteBuffer fillNoiseAccel(int minY, int height, int cellWidth, int cellHeight, int x, int z) {
        return engine.fillNoiseAccel(this.ptr, minY, height, cellWidth, cellHeight, x, z);
    }

    public void releaseDensities() {
        engine.releaseDensities(this.ptr);
    }
}
