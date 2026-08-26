package com.endrf.operations;

import java.util.Objects;

public class ConstantArg implements OperationArg, Spline.SplineArg {
    final double value;

    public ConstantArg(double value) {
        this.value = value;
    }

    @Override
    public int getHash() {
        return Objects.hash("const", this.value);
    }
}
