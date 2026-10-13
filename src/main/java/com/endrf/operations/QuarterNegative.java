package com.endrf.operations;

import java.util.Objects;

public class QuarterNegative implements OperationArg {
    public final static String TYPE = "minecraft:quarter_negative";
    private String type = TYPE;

    private OperationArg argument;

    public QuarterNegative(OperationArg argument) {
        this.argument = argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument.getHash());
    }
}
