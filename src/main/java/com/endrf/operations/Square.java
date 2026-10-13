package com.endrf.operations;

import java.util.Objects;

public class Square implements OperationArg {
    public final static String TYPE = "minecraft:square";
    private String type = TYPE;

    private OperationArg argument;

    public Square(OperationArg argument) {
        this.argument = argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument.getHash());
    }
}
