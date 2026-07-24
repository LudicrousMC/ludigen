package com.endrf.operations;

public class Noise implements OperationArg {
    public final static String TYPE = "minecraft:noise";
    private final String type = TYPE;

    private final String noise;
    public final double xzScale;
    public final double yScale;

    public Noise(String noise, double xzScale, double yScale) {
        this.noise = noise;
        this.xzScale = xzScale;
        this.yScale = yScale;
    }

    public String getNoisePath() {
        return this.noise;
    }
}
