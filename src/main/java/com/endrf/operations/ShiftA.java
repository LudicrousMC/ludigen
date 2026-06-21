package com.endrf.operations;

public class ShiftA implements OperationArg {
    public final static String TYPE = "minecraft:shift_a";
    private final String type = TYPE;

    private final String argument;

    public ShiftA(String argument) {
        this.argument = argument;
    }
}
