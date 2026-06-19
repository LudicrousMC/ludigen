package com.endrf.operations;

import com.google.gson.*;
import java.lang.reflect.Type;

public class OperationDeserializer implements JsonDeserializer<OperationArg> {
    @Override
    public OperationArg deserialize(JsonElement json, Type typeOfT, JsonDeserializationContext context) {
        if (json.isJsonPrimitive()) {
            try {
                return new ConstantArg(json.getAsDouble());
            } catch (NumberFormatException _) {
                return new ExternalArg(json.getAsString());
            }
        }

        JsonObject obj = json.getAsJsonObject();
        String type = obj.get("type").getAsString();

        return switch (type) {
            case Add.TYPE -> context.deserialize(json, Add.class);
            case Mul.TYPE -> context.deserialize(json, Mul.class);
            case Min.TYPE -> context.deserialize(json, Min.class);
            case Max.TYPE -> context.deserialize(json, Max.class);
            case Abs.TYPE -> context.deserialize(json, Abs.class);
            case Square.TYPE -> context.deserialize(json, Square.class);
            case Cube.TYPE -> context.deserialize(json, Cube.class);
            case HalfNegative.TYPE -> context.deserialize(json, HalfNegative.class);
            case QuarterNegative.TYPE -> context.deserialize(json, QuarterNegative.class);
            case Squeeze.TYPE -> context.deserialize(json, Squeeze.class);
            case Clamp.TYPE -> context.deserialize(json, Clamp.class);
            case YClampedGradient.TYPE -> context.deserialize(json, YClampedGradient.class);
            case RangeChoice.TYPE -> context.deserialize(json, RangeChoice.class);
            case IntervalSelect.TYPE -> context.deserialize(json, IntervalSelect.class);
            case Noise.TYPE -> context.deserialize(json, Noise.class);
            case ShiftedNoise.TYPE -> context.deserialize(json, ShiftedNoise.class);
            case Spline.TYPE -> context.deserialize(json, Spline.class);
            case BlendDensity.TYPE -> context.deserialize(json, BlendDensity.class);
            case BlendOffset.TYPE -> context.deserialize(json, BlendOffset.class);
            case BlendAlpha.TYPE -> context.deserialize(json, BlendAlpha.class);
            case Interpolated.TYPE -> context.deserialize(json, Interpolated.class);
            case CacheOnce.TYPE -> context.deserialize(json, CacheOnce.class);
            case FlatCache.TYPE -> context.deserialize(json, FlatCache.class);
            case Cache2D.TYPE -> context.deserialize(json, Cache2D.class);
            case ShiftA.TYPE -> context.deserialize(json, ShiftA.class);
            case ShiftB.TYPE -> context.deserialize(json, ShiftB.class);
            case OldBlendedNoise.TYPE -> context.deserialize(json, OldBlendedNoise.class);
            case EndIslands.TYPE -> context.deserialize(json, EndIslands.class);
            default -> throw new JsonParseException("Unknow density function type: " + type + " ");
        };
    }
}
