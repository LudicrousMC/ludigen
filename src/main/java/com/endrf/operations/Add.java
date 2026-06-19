package com.endrf.operations;

public class Add implements OperationArg {
    public final static String TYPE = "minecraft:add";
    private final String type = TYPE;
    private final OperationArg argument1;
    private final OperationArg argument2;

    public Add(OperationArg arg1, OperationArg arg2) {
        this.argument1 = arg1;
        this.argument2 = arg2;
    }
}
