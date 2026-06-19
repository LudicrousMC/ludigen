package com.endrf.operations;

public class Squeeze implements OperationArg {
    public final static String TYPE = "minecraft:squeeze";
    public final String type = TYPE;

    private final OperationArg argument;

    public Squeeze(OperationArg argument) {
        this.argument = argument;
    }
}
