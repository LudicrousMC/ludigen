package com.endrf.operations;

import com.google.gson.*;

import java.lang.reflect.Type;

public class Spline implements OperationArg {
    public final static String TYPE = "minecraft:spline";
    private final String type = TYPE;

    private final SplineArg spline;

    public Spline(SplineArg spline) {
        this.spline = spline;
    }


    public interface SplineArg {}
    static class SplineValue implements SplineArg {
        private final OperationArg coordinate;
        private final SplinePoint[] points;

        SplineValue(OperationArg coordinate, SplinePoint[] points) {
            this.coordinate = coordinate;
            this.points = points;
        }
    }

    static class SplinePoint {
        private final double derivative;
        private final double location;
        private final SplineArg value;

        SplinePoint(double derivative, double location, SplineArg value) {
            this.derivative = derivative;
            this.location = location;
            this.value = value;
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
