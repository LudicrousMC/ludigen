package com.endrf.operations;

public class FlatCache implements OperationArg {
    public final static String TYPE = "minecraft:flat_cache";
    private final String type = TYPE;

    private final OperationArg argument;

    public FlatCache(OperationArg argument) {
        this.argument = argument;
    }
}
