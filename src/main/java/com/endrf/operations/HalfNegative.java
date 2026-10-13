package com.endrf.operations;

import java.util.Objects;

public class HalfNegative implements OperationArg {
    public final static String TYPE = "minecraft:half_negative";
    private String type = TYPE;

    private OperationArg argument;

    public HalfNegative(OperationArg argument) {
        this.argument = argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument.getHash());
    }
}
