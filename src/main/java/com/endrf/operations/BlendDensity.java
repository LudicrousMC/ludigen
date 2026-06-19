package com.endrf.operations;

public class BlendDensity implements OperationArg {
    public final static String TYPE = "minecraft:blend_density";
    public final String type = TYPE;

    private final OperationArg argument;

    public BlendDensity(OperationArg argument) {
        this.argument = argument;
    }
}
