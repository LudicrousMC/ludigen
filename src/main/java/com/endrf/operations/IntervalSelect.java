package com.endrf.operations;

import java.util.Objects;

public class IntervalSelect implements OperationArg {
    public final static String TYPE = "minecraft:interval_select";
    private final String type = TYPE;

    private final OperationArg[] functions;
    private final OperationArg input;
    private final double[] thresholds;

    public IntervalSelect(OperationArg[] functions, OperationArg input, double[] thresholds) {
        this.functions = functions;
        this.input = input;
        this.thresholds = thresholds;
    }

    @Override
    public int getHash() {
        int hash = Objects.hash(this.type, this.input.getHash());
        for (Double d : this.thresholds) {
            hash += Objects.hash(d);
        }
        for (OperationArg a : this.functions) {
            hash += Objects.hash(a.getHash());
        }
        return hash;
    }
}
