package com.endrf.operations;

import com.endrf.JsonManager;
import com.endrf.NoiseStates;
import com.google.gson.*;
import net.minecraft.world.level.levelgen.PositionalRandomFactory;

import java.lang.reflect.Type;
import java.util.Map;

public class ShiftASerializer implements JsonSerializer<ShiftA> {
    PositionalRandomFactory rand;
    Map<String, JsonManager.NoiseParameter> noiseParams;
    String identifier;

    public ShiftASerializer(PositionalRandomFactory rand, Map<String, JsonManager.NoiseParameter> noiseParams, String identifier) {
        this.rand = rand;
        this.noiseParams = noiseParams;
        this.identifier = identifier;
    }

    @Override
    public JsonElement serialize(ShiftA src, Type typeOfSrc, JsonSerializationContext context) {
        int noiseIndex = NoiseStates.registerNoiseState(rand, src.getNoisePath(), noiseParams, identifier);
        JsonObject json = new JsonObject();
        json.addProperty("type", "minecraft:shift_a");
        json.addProperty("argument", noiseIndex);

        return json;
    }
}
