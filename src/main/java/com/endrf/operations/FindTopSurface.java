package com.endrf.operations;

import java.util.Objects;

public class FindTopSurface implements OperationArg {
    public final static String TYPE = "minecraft:find_top_surface";
    private String type = TYPE;

    private int cellHeight;
    private OperationArg density;
    private int lowerBound;
    private OperationArg upperBound;

    public FindTopSurface(int cellHeight, OperationArg density, int lowerBound, OperationArg upperBound) {
        this.cellHeight = cellHeight;
        this.density = density;
        this.lowerBound = lowerBound;
        this.upperBound = upperBound;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.cellHeight, this.lowerBound, this.density.getHash(), this.upperBound.getHash());
    }
}
