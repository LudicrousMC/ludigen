package com.endrf.operations;

import java.util.Objects;

public class Noise implements OperationArg {
    public final static String TYPE = "minecraft:noise";
    private String type = TYPE;

    private String noise;
    public double xzScale;
    public double yScale;

    public Noise(String noise, double xzScale, double yScale) {
        this.noise = noise;
        this.xzScale = xzScale;
        this.yScale = yScale;
    }

    public String getNoisePath() {
        return this.noise;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.xzScale, this.yScale, this.noise);
    }
}
