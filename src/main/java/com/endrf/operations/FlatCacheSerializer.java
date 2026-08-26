package com.endrf.operations;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonSerializationContext;
import com.google.gson.JsonSerializer;

import java.lang.reflect.Type;
import java.util.HashMap;

public class FlatCacheSerializer implements JsonSerializer<FlatCache> {
    private HashMap<Integer, Integer> functionMap = new HashMap<>();
    private int numFunctions = 0;

    @Override
    public JsonElement serialize(FlatCache src, Type typeOfSrc, JsonSerializationContext context) {
        int hash = src.argument.getHash();
        int index;
        if (this.functionMap.containsKey(hash)) {
            index = this.functionMap.get(hash);
        } else {
            index = numFunctions++;
            functionMap.put(hash, index);
        }
        JsonObject json = new JsonObject();
        json.addProperty("type", "minecraft:flat_cache");
        json.add("argument", context.serialize(src.argument));
        json.addProperty("index", index);
        return json;
    }
}
