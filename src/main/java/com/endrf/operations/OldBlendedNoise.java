package com.endrf.operations;

public class OldBlendedNoise implements OperationArg {
    public final static String TYPE = "minecraft:old_blended_noise";
    private final String type = TYPE;

    private final double smearScaleMultiplier;
    private final double xzFactor;
    private final double xzScale;
    private final double yFactor;
    private final double yScale;

    public OldBlendedNoise(double smearScaleMultiplier, double xzFactor, double xzScale, double yFactor, double yScale) {
        this.smearScaleMultiplier = smearScaleMultiplier;
        this.xzFactor = xzFactor;
        this.xzScale = xzScale;
        this.yFactor = yFactor;
        this.yScale = yScale;
    }
}
