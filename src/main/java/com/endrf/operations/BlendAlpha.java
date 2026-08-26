package com.endrf.operations;

import java.util.Objects;

public class BlendAlpha implements OperationArg {
    public final static String TYPE = "minecraft:blend_alpha";
    private final String type = TYPE;

    @Override
    public int getHash() {
        return Objects.hash(this.type);
    }
}
