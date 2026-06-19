package com.endrf.operations;

public class Clamp implements OperationArg {
    public final static String TYPE = "minecraft:clamp";
    private final String type = TYPE;

    private final OperationArg input;
    private final double min;
    private final double max;

    public Clamp(OperationArg input, double min, double max) {
        this.input = input;
        this.min = min;
        this.max = max;
    }
}
