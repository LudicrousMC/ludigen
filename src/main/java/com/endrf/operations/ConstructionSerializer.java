package com.endrf.operations;

import com.endrf.JsonManager;
import com.google.gson.JsonElement;
import com.google.gson.JsonSerializationContext;
import com.google.gson.JsonSerializer;

import java.lang.reflect.Type;

public class ConstructionSerializer implements JsonSerializer<OperationArg> {
    @Override
    public JsonElement serialize(OperationArg src, Type typeOfSrc, JsonSerializationContext context) {
        if (src instanceof ExternalArg) {
            return context.serialize(JsonManager.densityFunctions.get(((ExternalArg) src).path));
        }
        JsonElement prim = OperationSerializer.serializePrimitives(src, context);
        return prim != null ? prim : context.serialize(src);
    }
}
