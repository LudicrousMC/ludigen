package com.endrf.operations;

import java.util.Objects;

public class EndIslands implements OperationArg {
    public final static String TYPE = "minecraft:end_islands";
    private String type = TYPE;

    @Override
    public int getHash() {
        return Objects.hash(this.type);
    }
}
