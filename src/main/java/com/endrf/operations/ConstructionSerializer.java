package com.endrf.operations;

import com.endrf.JsonManager;
import com.endrf.NoiseStates;
import com.google.gson.*;
import net.minecraft.util.RandomSource;
import net.minecraft.world.level.levelgen.PositionalRandomFactory;

import java.lang.reflect.Type;
import java.util.HashMap;
import java.util.Map;

public class ConstructionSerializer implements JsonSerializer<OperationArg> {
    PositionalRandomFactory rand;
    Map<String, JsonManager.NoiseParameter> noiseParams;

    public ConstructionSerializer(PositionalRandomFactory rand, Map<String, JsonManager.NoiseParameter> noiseParams) {
        this.rand = rand;
        this.noiseParams = noiseParams;
    }

    @Override
    public JsonElement serialize(OperationArg src, Type typeOfSrc, JsonSerializationContext context) {
        if (src instanceof ExternalArg) {
            return context.serialize(JsonManager.densityFunctions.get(((ExternalArg) src).path));
        }

        /*switch ((Object)src) {
            case Noise n -> {
                String noise = n.getNoisePath();
                NoiseStates.registerNoiseState(rand, noise, noiseParams);
            }
            case ShiftedNoise n -> {
                String noise = n.getNoisePath();
                System.out.println(noise);
                NoiseStates.registerNoiseState(rand, noise, noiseParams);
            }
            case ShiftA n -> {
                String noise = n.getNoisePath();
                NoiseStates.registerNoiseState(rand, noise, noiseParams);
            }
            case ShiftB n -> {
                String noise = n.getNoisePath();
                NoiseStates.registerNoiseState(rand, noise, noiseParams);
            }
            default -> {}
        }*/
        JsonElement prim = OperationSerializer.serializePrimitives(src, context);
        return prim != null ? prim : context.serialize(src);
    }
}
