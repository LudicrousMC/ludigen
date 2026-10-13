package com.endrf.operations;

import java.util.Objects;

public class Abs implements OperationArg {
    public final static String TYPE = "minecraft:abs";
    private String type = TYPE;

    private OperationArg argument;

    public Abs(OperationArg argument) {
        this.argument = argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument.getHash());
    }
}
