package com.endrf.operations;

import com.endrf.JsonManager;
import com.endrf.NoiseStates;
import com.google.gson.*;
import net.minecraft.world.level.levelgen.PositionalRandomFactory;

import java.lang.reflect.Type;
import java.util.Map;

public class NoiseSerializer implements JsonSerializer<Noise> {
    PositionalRandomFactory rand;
    Map<String, JsonManager.NoiseParameter> noiseParams;
    String identifier;

    public NoiseSerializer(PositionalRandomFactory rand, Map<String, JsonManager.NoiseParameter> noiseParams, String identifier) {
        this.rand = rand;
        this.noiseParams = noiseParams;
        this.identifier = identifier;
    }

    @Override
    public JsonElement serialize(Noise src, Type typeOfSrc, JsonSerializationContext context) {
        int noiseIndex = NoiseStates.registerNoiseState(rand, src.getNoisePath(), noiseParams, this.identifier);
        JsonObject json = new JsonObject();
        json.addProperty("type", "minecraft:noise");
        json.addProperty("y_scale", src.yScale);
        json.addProperty("xz_scale", src.xzScale);
        json.addProperty("noise", noiseIndex);

        return json;
    }
}
