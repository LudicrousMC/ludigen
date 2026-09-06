package com.endrf.operations;

import com.endrf.JsonManager;
import com.endrf.NoiseStates;
import com.google.gson.*;
import net.minecraft.world.level.levelgen.PositionalRandomFactory;

import java.lang.reflect.Type;
import java.util.Map;

public class ShiftedNoiseSerializer implements JsonSerializer<ShiftedNoise> {
    PositionalRandomFactory rand;
    Map<String, JsonManager.NoiseParameter> noiseParams;
    String identifier;

    public ShiftedNoiseSerializer(PositionalRandomFactory rand, Map<String, JsonManager.NoiseParameter> noiseParams, String identifier) {
        this.rand = rand;
        this.noiseParams = noiseParams;
        this.identifier = identifier;
    }

    @Override
    public JsonElement serialize(ShiftedNoise src, Type typeOfSrc, JsonSerializationContext context) {
        int noiseIndex = NoiseStates.registerNoiseState(rand, src.getNoisePath(), noiseParams, this.identifier);
        JsonObject json = new JsonObject();
        json.addProperty("type", "minecraft:shifted_noise");
        json.add("shift_x", context.serialize(src.shiftX));
        json.add("shift_y", context.serialize(src.shiftY));
        json.add("shift_z", context.serialize(src.shiftZ));
        json.addProperty("y_scale", src.yScale);
        json.addProperty("xz_scale", src.xzScale);
        json.addProperty("noise", noiseIndex);

        return json;
    }
}
