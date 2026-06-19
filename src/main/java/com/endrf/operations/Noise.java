package com.endrf.operations;

public class Noise implements OperationArg {
    public final static String TYPE = "minecraft:noise";
    private final String type = TYPE;

    private final String noise;
    private final double xzScale;
    private final double yScale;

    public Noise(String noise, double xzScale, double yScale) {
        this.noise = noise;
        this.xzScale = xzScale;
        this.yScale = yScale;
    }
}
