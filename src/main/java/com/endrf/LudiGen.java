package com.endrf;

import com.google.common.base.Stopwatch;
import net.fabricmc.api.ModInitializer;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerLevelEvents;
import net.minecraft.server.Main;

import net.minecraft.server.MinecraftServer;
import net.minecraft.server.level.ServerLevel;
import net.minecraft.world.level.chunk.ChunkGenerator;
import net.minecraft.world.level.levelgen.NoiseBasedChunkGenerator;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicReference;

public class LudiGen implements ModInitializer {
	public static final String MOD_ID = "ludigen";

	public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);
    private static final Logger NATIVE_LOGGER = LoggerFactory.getLogger(MOD_ID + "/Engine");

    public static HashMap<String, String> DIM_MAPPING = new HashMap<>(Map.of(
            "nether", "the_nether",
            "end", "the_end"
    ));

    public static AtomicReference<Long> seed = new AtomicReference<>();
    private static JsonManager json;
    public static LudicrousEngineInstance instance;

    public static int testCounter = -1;
    public static long testStart = 0;
    public static long totalTime = 0;
    public static long totalChunks = 0;

	@Override
	public void onInitialize() {
        instance = LudicrousEngine.createInstance();
        ServerLevelEvents.LOAD.register((MinecraftServer server, ServerLevel level) -> {
            if (seed.get() == null) {
                seed.set(server.getWorldGenSettings().options().seed());
                try {
                    Stopwatch sw = Stopwatch.createStarted();
                    json = new JsonManager(Main.class);
                    LOGGER.info("Serialized terrain density JSON files and generated Noise States in {}", sw);
                } catch (Exception e) {
                    throw new RuntimeException(e);
                }

            }
            ChunkGenerator generator = level.getChunkSource().getGenerator();
            if (generator instanceof NoiseBasedChunkGenerator gen) {
                String dim = level.dimension().identifier().getPath();
                NoiseStates.NoiseStatesHolder noiseStatesData = NoiseStates.generateNoiseStates(dim + ":final_density");
                String fnJson = json.worldTypes.get(dim).jsonDensityFunctions.get("final_density");
                instance.loadFinalDensityShader(
                        dim,
                        3,
                        level.getMinY(),
                        level.getLogicalHeight(),
                        gen.generatorSettings().value().noiseSettings().noiseSizeHorizontal(),
                        gen.generatorSettings().value().noiseSettings().noiseSizeVertical(),
                        fnJson,
                        noiseStatesData.noiseStates,
                        noiseStatesData.noiseLevels
                );
            }
        });

	}

    public static void nativeLogInfoInterface(String message) {
        NATIVE_LOGGER.info(message);
    }

    public static void nativeLogErrorInterface(String message) {
        NATIVE_LOGGER.error(message);
    }

    public static void nativeLogDebugInterface(String message) {
        NATIVE_LOGGER.debug(message);
    }

    public static void inc() {
        if (testCounter == -2)
            return;
        if (testCounter == -1) {
            testStart = System.nanoTime();
            testCounter = 0;
        }
        testCounter++;
        if (testCounter == 4200) {
            testCounter = -2;
            long end = System.nanoTime();
            System.out.println("total t: " + (end - testStart));
        }
    }
}