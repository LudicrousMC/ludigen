package com.endrf.operations;

public class HalfNegative implements OperationArg {
    public final static String TYPE = "minecraft:half_negative";
    private final String type = TYPE;

    private final OperationArg argument;

    public HalfNegative(OperationArg argument) {
        this.argument = argument;
    }
}
