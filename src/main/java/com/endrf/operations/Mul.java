package com.endrf.operations;

public class Mul implements OperationArg {
    public final static String TYPE = "minecraft:mul";
    private final String type = TYPE;

    private final OperationArg argument1;
    private final OperationArg argument2;

    public Mul(OperationArg argument1, OperationArg argument2) {
        this.argument1 = argument1;
        this.argument2 = argument2;
    }
}
