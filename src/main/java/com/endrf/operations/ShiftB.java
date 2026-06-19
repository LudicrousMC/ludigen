package com.endrf.operations;

public class ShiftB implements OperationArg {
    public final static String TYPE = "minecraft:shift_b";
    private final String type = TYPE;

    private final OperationArg argument;

    public ShiftB(OperationArg argument) {
        this.argument = argument;
    }
}
