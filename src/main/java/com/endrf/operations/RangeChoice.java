package com.endrf.operations;

public class RangeChoice implements OperationArg {
    public final static String TYPE = "minecraft:range_choice";
    private final String type = TYPE;

    private final OperationArg input;
    private final double minInclusive;
    private final double maxExclusive;
    private final OperationArg whenInRange;
    private final OperationArg whenOutOfRange;


    public RangeChoice(OperationArg input, double minInclusive, double maxExclusive, OperationArg whenInRange, OperationArg whenOutOfRange) {
        this.input = input;
        this.minInclusive = minInclusive;
        this.maxExclusive = maxExclusive;
        this.whenInRange = whenInRange;
        this.whenOutOfRange = whenOutOfRange;
    }
}
