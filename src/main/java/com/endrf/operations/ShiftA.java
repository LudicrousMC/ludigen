package com.endrf.operations;

import java.util.Objects;

public class ShiftA implements OperationArg {
    public final static String TYPE = "minecraft:shift_a";
    private String type = TYPE;

    private String argument;

    public ShiftA(String argument) {
        this.argument = argument;
    }

    public String getNoisePath() {
        return this.argument;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.argument);
    }
}
