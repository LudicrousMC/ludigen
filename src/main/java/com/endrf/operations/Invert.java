package com.endrf.operations;

import java.util.Objects;

public class Invert implements OperationArg {
    public final static String TYPE = "minecraft:invert";
    private final String type = TYPE;

    private final OperationArg argument;

    public Invert(OperationArg argument) {
        this.argument = argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument.getHash());
    }
}
