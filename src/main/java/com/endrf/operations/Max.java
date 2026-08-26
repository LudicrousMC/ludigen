package com.endrf.operations;

import java.util.Objects;

public class Max implements OperationArg {
    public final static String TYPE = "minecraft:max";
    private final String type = TYPE;

    private final OperationArg argument1;
    private final OperationArg argument2;

    public Max(OperationArg argument1, OperationArg argument2) {
        this.argument1 = argument1;
        this.argument2 = argument2;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument1.getHash(), this.argument2.getHash());
    }
}
