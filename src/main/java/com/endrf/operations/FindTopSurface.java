package com.endrf.operations;

public class FindTopSurface implements OperationArg {
    public final static String TYPE = "minecraft:find_top_surface";
    private final String type = TYPE;

    private final int cellHeight;
    private final OperationArg density;
    private final int lowerBound;
    private final OperationArg upperBound;

    public FindTopSurface(int cellHeight, OperationArg density, int lowerBound, OperationArg upperBound) {
        this.cellHeight = cellHeight;
        this.density = density;
        this.lowerBound = lowerBound;
        this.upperBound = upperBound;
    }
}
