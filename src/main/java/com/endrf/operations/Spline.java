package com.endrf.operations;

import com.google.gson.*;

import java.lang.reflect.Type;
import java.util.Objects;

public class Spline implements OperationArg {
    public final static String TYPE = "minecraft:spline";
    private String type = TYPE;

    private SplineArg spline;

    public Spline(SplineArg spline) {
        this.spline = spline;
    }

    @Override
    public int getHash() {
        return Objects.hash(this.type, this.spline.getHash());
    }


    public interface SplineArg {
        int getHash();
    }
    static class SplineValue implements SplineArg {
        private OperationArg coordinate;
        private SplinePoint[] points;

        SplineValue(OperationArg coordinate, SplinePoint[] points) {
            this.coordinate = coordinate;
            this.points = points;
        }

        @Override
        public int getHash() {
            int hash = Objects.hash("spline", this.coordinate.getHash());
            for (SplinePoint p : this.points) {
                hash += p.getHash();
            }
            return hash;
        }
    }

    static class SplinePoint {
        private double derivative;
        private double location;
        private SplineArg value;

        SplinePoint(double derivative, double location, SplineArg value) {
            this.derivative = derivative;
            this.location = location;
            this.value = value;
        }

        int getHash() {
            return Objects.hash("sp", this.derivative, this.location, this.value.getHash());
        }
    }

    public static class SplineDeserializer implements JsonDeserializer<SplineArg> {
        @Override
        public SplineArg deserialize(JsonElement json, Type typeOfT, JsonDeserializationContext context) throws JsonParseException {
            if (json.isJsonPrimitive()) {
                return new ConstantArg(json.getAsDouble());
            }

            return context.deserialize(json, SplineValue.class);
        }
    }
}
