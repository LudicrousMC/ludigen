package com.endrf.operations;

import java.util.Objects;

public class Cube implements OperationArg {
    public final static String TYPE = "minecraft:cube";
    private String type = TYPE;

    private OperationArg argument;

    public Cube(OperationArg argument) {
        this.argument = argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument.getHash());
    }
}
