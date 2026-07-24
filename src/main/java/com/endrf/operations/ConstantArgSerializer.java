package com.endrf.operations;

import com.google.gson.JsonElement;
import com.google.gson.JsonPrimitive;
import com.google.gson.JsonSerializationContext;
import com.google.gson.JsonSerializer;

import java.lang.reflect.Type;

public class ConstantArgSerializer implements JsonSerializer<ConstantArg> {
    @Override
    public JsonElement serialize(ConstantArg src, Type typeOfSrc, JsonSerializationContext context) {
        return new JsonPrimitive(src.value);
    }
}
