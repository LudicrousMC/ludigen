package com.endrf.operations;

public class Invert implements OperationArg {
    public final static String TYPE = "minecraft:invert";
    private final String type = TYPE;

    private final OperationArg argument;

    public Invert(OperationArg argument) {
        this.argument = argument;
    }
}
