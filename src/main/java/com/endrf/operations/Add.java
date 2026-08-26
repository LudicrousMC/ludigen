package com.endrf.operations;

import java.util.Objects;

public class Add implements OperationArg {
    public final static String TYPE = "minecraft:add";
    private final String type = TYPE;
    private final OperationArg argument1;
    private final OperationArg argument2;

    public Add(OperationArg arg1, OperationArg arg2) {
        this.argument1 = arg1;
        this.argument2 = arg2;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument1.getHash(), this.argument2.getHash());
    }
}
