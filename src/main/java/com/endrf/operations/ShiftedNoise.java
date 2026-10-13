package com.endrf.operations;

import java.util.Objects;

public class ShiftedNoise implements OperationArg {
    public final static String TYPE = "minecraft:shifted_noise";
    private String type = TYPE;

    private String noise;
    public double xzScale;
    public double yScale;
    public OperationArg shiftX;
    public OperationArg shiftY;
    public OperationArg shiftZ;

    public ShiftedNoise(String noise, double xzScale, double yScale, OperationArg shiftX, OperationArg shiftY, OperationArg shiftZ) {
        this.noise = noise;
        this.xzScale = xzScale;
        this.yScale = yScale;
        this.shiftX = shiftX;
        this.shiftY = shiftY;
        this.shiftZ = shiftZ;
    }

    public String getNoisePath() {
        return this.noise;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.xzScale, this.yScale, this.noise, this.shiftX.getHash(), this.shiftY.getHash(), this.shiftZ.getHash());
    }
}
