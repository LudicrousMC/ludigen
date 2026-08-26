package com.endrf.operations;

import java.util.Objects;

public class Interpolated implements OperationArg {
    public final static String TYPE = "minecraft:interpolated";
    private final String type = TYPE;

    private final OperationArg argument;

    public Interpolated(OperationArg argument) {
        this.argument = argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument.getHash());
    }
}
