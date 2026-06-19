package com.endrf.operations;

public class QuarterNegative implements OperationArg {
    public final static String TYPE = "minecraft:quarter_negative";
    private final String type = TYPE;

    private final OperationArg argument;

    public QuarterNegative(OperationArg argument) {
        this.argument = argument;
    }
}
