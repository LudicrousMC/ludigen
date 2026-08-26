package com.endrf.operations;

import java.util.Objects;

public class BlendDensity implements OperationArg {
    public final static String TYPE = "minecraft:blend_density";
    public final String type = TYPE;

    private final OperationArg argument;

    public BlendDensity(OperationArg argument) {
        this.argument = argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument.getHash());
    }
}
