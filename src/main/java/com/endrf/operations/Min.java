package com.endrf.operations;

public class Min implements OperationArg {
    public final static String TYPE = "minecraft:min";
    private final String type = TYPE;

    private final OperationArg argument1;
    private final OperationArg argument2;

    public Min(OperationArg argument1, OperationArg argument2) {
        this.argument1 = argument1;
        this.argument2 = argument2;
    }
}
