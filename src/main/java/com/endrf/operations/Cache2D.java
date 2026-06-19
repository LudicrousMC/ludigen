package com.endrf.operations;

public class Cache2D implements OperationArg {
    public final static String TYPE = "minecraft:cache_2d";
    private final String type = TYPE;

    private final OperationArg argument;

    public Cache2D(OperationArg argument) {
        this.argument = argument;
    }
}
