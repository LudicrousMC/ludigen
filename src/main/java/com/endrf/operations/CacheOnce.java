package com.endrf.operations;

import java.util.Objects;

public class CacheOnce implements OperationArg {
    public final static String TYPE = "minecraft:cache_once";
    private String type = TYPE;

    public OperationArg argument;

    public CacheOnce(OperationArg argument) {
        this.argument = argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument.getHash());
    }
}
