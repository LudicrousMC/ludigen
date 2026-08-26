package com.endrf.operations;

import java.util.Objects;

public class ExternalArg implements OperationArg {
    public final String path;

    public ExternalArg(String path) {
        this.path = path;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.path);
    }
}
