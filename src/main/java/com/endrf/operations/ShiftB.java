package com.endrf.operations;

public class ShiftB implements OperationArg {
    public final static String TYPE = "minecraft:shift_b";
    private final String type = TYPE;

    private final String argument;

    public ShiftB(String argument) {
        this.argument = argument;
    }
}
