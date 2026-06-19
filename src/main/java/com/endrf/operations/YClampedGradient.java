package com.endrf.operations;

public class YClampedGradient implements OperationArg {
    public final static String TYPE = "minecraft:y_clamped_gradient";
    private final String type = TYPE;
    final double fromValue;
    final double toValue;
    final int fromY;
    final int toY;

    public YClampedGradient(double fromValue, double toValue, int fromY, int toY) {
        this.fromValue = fromValue;
        this.toValue = toValue;
        this.fromY = fromY;
        this.toY = toY;
    }
}
