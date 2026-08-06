package com.endrf;

import net.fabricmc.api.ModInitializer;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerLevelEvents;
import net.minecraft.server.Main;

import net.minecraft.server.MinecraftServer;
import net.minecraft.server.level.ServerLevel;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicReference;

public class LudiGen implements ModInitializer {
	public static final String MOD_ID = "ludigen";

	// This logger is used to write text to the console and the log file.
	// It is considered best practice to use your mod id as the logger's name.
	// That way, it's clear which mod wrote info, warnings, and errors.
	public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);

    public static AtomicReference<Long> seed = new AtomicReference<>();
    public static HashMap<String, Integer> dimensions = new HashMap<>();
    public static LudicrousEngineInstance instance = LudicrousEngine.createInstance();

	@Override
	public void onInitialize() {
		// This code runs as soon as Minecraft is in a mod-load-ready state.
		// However, some things (like resources) may still be uninitialized.
		// Proceed with mild caution.
        ServerLevelEvents.LOAD.register((MinecraftServer server, ServerLevel level) -> {
            if (seed.get() == null) {
                seed.set(server.getWorldGenSettings().options().seed());
                System.out.println("seed " + seed);
                startInstance();
            }
            long seed = LudiGen.seed.get();
            String dim = level.dimension().identifier().getPath();
            //dimensions.put(dim, );
            System.out.println(level.dimension().identifier().getPath());
        });


		/*String json;
		try (InputStream in = Main.class.getResourceAsStream("/data/minecraft/worldgen/density_function/overworld_amplified/depth.json")) {
			if (in == null) throw new RuntimeException("Not found");
			json = new String(in.readAllBytes(), StandardCharsets.UTF_8);
		} catch (RuntimeException | IOException e) {
			throw new RuntimeException(e);
		}
		Gson gson = new GsonBuilder().registerTypeAdapter(OperationArg.class, new OperationDeserializer())
				.setFieldNamingPolicy(FieldNamingPolicy.LOWER_CASE_WITH_UNDERSCORES).create();
		LOGGER.info(json);
		OperationArg res = gson.fromJson(json, OperationArg.class);
        //LOGGER.info(((ExternalArg)(((Add)res).argument2)).path);
		LOGGER.info("res = " + gson.toJson(res));*/


        //LOGGER.info("ptr " + instance);

        LOGGER.info("Hello Fabric world!");
	}

    public void startInstance() {
        NoiseStates.addOldBlendedNoise();
        JsonManager json;
        try {
            json = new JsonManager(Main.class);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
        instance = LudicrousEngine.createInstance();
        instance.setDensityFunction(json.worldTypes.get("overworld").jsonDensityFunctions.get("final_density"));
        instance.setNoiseStates(NoiseStates.noiseStates, NoiseStates.noiseLevelStates);
        //LudicrousEngine.test();
        try {
            instance.registerShader("eval");
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}