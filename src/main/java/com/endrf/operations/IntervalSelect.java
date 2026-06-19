package com.endrf.operations;

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
}
