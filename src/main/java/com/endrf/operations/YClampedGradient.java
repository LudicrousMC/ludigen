package com.endrf.operations;

import java.util.Objects;

public class YClampedGradient implements OperationArg {
    public final static String TYPE = "minecraft:y_clamped_gradient";
    private String type = TYPE;
    double fromValue;
    double toValue;
    int fromY;
    int toY;

    public YClampedGradient(double fromValue, double toValue, int fromY, int toY) {
        this.fromValue = fromValue;
        this.toValue = toValue;
        this.fromY = fromY;
        this.toY = toY;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.fromValue, this.toValue, this.fromY, this.toY);
    }
}
