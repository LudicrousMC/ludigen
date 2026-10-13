package com.endrf.operations;

import java.util.Objects;

public class OldBlendedNoise implements OperationArg {
    public final static String TYPE = "minecraft:old_blended_noise";
    private String type = TYPE;

    private double smearScaleMultiplier;
    private double xzFactor;
    private double xzScale;
    private double yFactor;
    private double yScale;

    public OldBlendedNoise(double smearScaleMultiplier, double xzFactor, double xzScale, double yFactor, double yScale) {
        this.smearScaleMultiplier = smearScaleMultiplier;
        this.xzFactor = xzFactor;
        this.xzScale = xzScale;
        this.yFactor = yFactor;
        this.yScale = yScale;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.smearScaleMultiplier, this.xzFactor, this.xzScale, this.yFactor, this.yScale);
    }
}
