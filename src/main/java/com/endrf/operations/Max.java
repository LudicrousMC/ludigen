package com.endrf.operations;

public class Max implements OperationArg {
    public final static String TYPE = "minecraft:max";
    private final String type = TYPE;

    private final OperationArg argument1;
    private final OperationArg argument2;

    public Max(OperationArg argument1, OperationArg argument2) {
        this.argument1 = argument1;
        this.argument2 = argument2;
    }
}
