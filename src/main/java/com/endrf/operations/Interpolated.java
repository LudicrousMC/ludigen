package com.endrf.operations;

public class Interpolated implements OperationArg {
    public final static String TYPE = "minecraft:interpolated";
    private final String type = TYPE;

    private final OperationArg argument;

    public Interpolated(OperationArg argument) {
        this.argument = argument;
    }
}
