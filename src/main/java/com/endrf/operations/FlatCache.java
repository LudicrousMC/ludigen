package com.endrf.operations;

import java.util.Objects;

public class FlatCache implements OperationArg {
    public final static String TYPE = "minecraft:flat_cache";
    private final String type = TYPE;

    public final OperationArg argument;

    public FlatCache(OperationArg argument) {
        this.argument = argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument.getHash());
    }
}
