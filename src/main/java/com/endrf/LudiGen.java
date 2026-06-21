package com.endrf;

import com.endrf.operations.*;
import com.google.gson.FieldNamingPolicy;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import net.fabricmc.api.ModInitializer;
import net.minecraft.server.Main;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Modifier;
import java.nio.charset.StandardCharsets;

public class LudiGen implements ModInitializer {
	public static final String MOD_ID = "ludigen";

	// This logger is used to write text to the console and the log file.
	// It is considered best practice to use your mod id as the logger's name.
	// That way, it's clear which mod wrote info, warnings, and errors.
	public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);

	@Override
	public void onInitialize() {
		// This code runs as soon as Minecraft is in a mod-load-ready state.
		// However, some things (like resources) may still be uninitialized.
		// Proceed with mild caution.


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
        long start = System.nanoTime();
        try {
            JsonManager json = new JsonManager(Main.class);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
        LudicrousEngineInstance instance = LudicrousEngine.createInstance();
        long end = System.nanoTime();
        LOGGER.info("time: " + (end - start));
        //LudicrousEngine.test();
        try {
            instance.registerShader("eval");
        } catch (IOException e) {
            throw new RuntimeException(e);
        }

        //LOGGER.info("ptr " + instance);

        LOGGER.info("Hello Fabric world!");
	}
}