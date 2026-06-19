package com.endrf.operations;

public class CacheOnce implements OperationArg {
    public final static String TYPE = "minecraft:cache_once";
    private final String type = TYPE;

    private final OperationArg argument;

    public CacheOnce(OperationArg argument) {
        this.argument = argument;
    }
}
