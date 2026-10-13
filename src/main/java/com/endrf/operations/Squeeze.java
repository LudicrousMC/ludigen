package com.endrf.operations;

import java.util.Objects;

public class Squeeze implements OperationArg {
    public final static String TYPE = "minecraft:squeeze";
    public String type = TYPE;

    private OperationArg argument;

    public Squeeze(OperationArg argument) {
        this.argument = argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument.getHash());
    }
}
