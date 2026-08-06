package com.endrf.operations;

import com.endrf.JsonManager;
import com.endrf.NoiseStates;
import com.google.gson.*;
import net.minecraft.world.level.levelgen.PositionalRandomFactory;

import java.lang.reflect.Type;
import java.util.Map;

public class ShiftBSerializer implements JsonSerializer<ShiftB> {
    PositionalRandomFactory rand;
    Map<String, JsonManager.NoiseParameter> noiseParams;

    public ShiftBSerializer(PositionalRandomFactory rand, Map<String, JsonManager.NoiseParameter> noiseParams) {
        this.rand = rand;
        this.noiseParams = noiseParams;
    }

    @Override
    public JsonElement serialize(ShiftB src, Type typeOfSrc, JsonSerializationContext context) {
        int noiseIndex = NoiseStates.registerNoiseState(rand, src.getNoisePath(), noiseParams);
        JsonObject json = new JsonObject();
        json.addProperty("type", "minecraft:shift_a");
        json.addProperty("argument", noiseIndex);

        return json;
    }
}
