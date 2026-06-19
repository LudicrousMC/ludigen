package com.endrf.operations;

public class Cube implements OperationArg {
    public final static String TYPE = "minecraft:cube";
    private final String type = TYPE;

    private final OperationArg argument;

    public Cube(OperationArg argument) {
        this.argument = argument;
    }
}
