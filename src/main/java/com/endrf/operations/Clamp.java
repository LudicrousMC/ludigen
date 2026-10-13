package com.endrf.operations;

import java.util.Objects;

public class Clamp implements OperationArg {
    public final static String TYPE = "minecraft:clamp";
    private String type = TYPE;

    private OperationArg input;
    private double min;
    private double max;

    public Clamp(OperationArg input, double min, double max) {
        this.input = input;
        this.min = min;
        this.max = max;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.min, this.max, this.input.getHash());
    }
}
