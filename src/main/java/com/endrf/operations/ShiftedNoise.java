package com.endrf.operations;

public class ShiftedNoise implements OperationArg {
    public final static String TYPE = "minecraft:shifted_noise";
    private final String type = TYPE;

    private final String noise;
    public final double xzScale;
    public final double yScale;
    public final OperationArg shiftX;
    public final OperationArg shiftY;
    public final OperationArg shiftZ;

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
}
