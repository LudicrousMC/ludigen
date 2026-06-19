package com.endrf.operations;

public class Abs implements OperationArg {
    public final static String TYPE = "minecraft:abs";
    private final String type = TYPE;

    private final OperationArg argument;

    public Abs(OperationArg argument) {
        this.argument = argument;
    }
}
