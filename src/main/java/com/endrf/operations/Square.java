package com.endrf.operations;

public class Square implements OperationArg {
    public final static String TYPE = "minecraft:square";
    private final String type = TYPE;

    private final OperationArg argument;

    public Square(OperationArg argument) {
        this.argument = argument;
    }
}
