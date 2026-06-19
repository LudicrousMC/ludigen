package com.endrf.operations;

public class ConstantArg implements OperationArg, Spline.SplineArg {
    final double value;

    public ConstantArg(double value) {
        this.value = value;
    }
}
