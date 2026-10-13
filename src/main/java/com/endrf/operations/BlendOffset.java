package com.endrf.operations;

import java.util.Objects;

public class BlendOffset implements OperationArg {
    public final static String TYPE = "minecraft:blend_offset";
    private String type = TYPE;

    @Override
    public int getHash() {
        return Objects.hash(this.type);
    }
}
