package com.endrf.operations;

public class ExternalArg implements OperationArg {
    public final String path;

    public ExternalArg(String path) {
        this.path = path;
    }
}
