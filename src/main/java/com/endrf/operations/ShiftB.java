package com.endrf.operations;

import java.util.Objects;

public class ShiftB implements OperationArg {
    public final static String TYPE = "minecraft:shift_b";
    private String type = TYPE;

    private String argument;

    public ShiftB(String argument) {
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
