package com.endrf.operations;

import java.util.Objects;

public class Cache2D implements OperationArg {
    public final static String TYPE = "minecraft:cache_2d";
    private final String type = TYPE;

    public final OperationArg argument;

    public Cache2D(OperationArg argument) {
        this.argument = argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument.getHash());
    }
}
