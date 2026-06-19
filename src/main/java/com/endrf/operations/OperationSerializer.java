package com.endrf.operations;

import com.google.gson.JsonElement;
import com.google.gson.JsonPrimitive;
import com.google.gson.JsonSerializationContext;
import com.google.gson.JsonSerializer;

import java.lang.reflect.Type;

public class OperationSerializer implements JsonSerializer<OperationArg> {
    @Override
    public JsonElement serialize(OperationArg src, Type typeOfSrc, JsonSerializationContext context) {
        JsonElement prim = serializePrimitives(src, context);
        return prim != null ? prim : context.serialize(src);
    }

    public static JsonElement serializePrimitives(OperationArg src, JsonSerializationContext context) {
        if (src instanceof ExternalArg) {
            return new JsonPrimitive(((ExternalArg) src).path);
        } else if (src instanceof ConstantArg) {
            return new JsonPrimitive(((ConstantArg) src).value);
        }
        return null;
    }
}
