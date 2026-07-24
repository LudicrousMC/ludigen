package com.endrf.operations;

import com.endrf.JsonManager;
import com.google.gson.JsonElement;
import com.google.gson.JsonSerializationContext;
import com.google.gson.JsonSerializer;

import java.lang.reflect.Type;

public class ExternalArgSerializer implements JsonSerializer<ExternalArg> {
    @Override
    public JsonElement serialize(ExternalArg src, Type typeOfSrc, JsonSerializationContext context) {
        return context.serialize(JsonManager.densityFunctions.get(src.path));
    }
}
