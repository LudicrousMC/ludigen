package com.endrf;

import com.endrf.operations.*;
import com.google.gson.*;
import net.minecraft.server.Main;
import net.minecraft.util.RandomSource;
import net.minecraft.world.level.levelgen.LegacyRandomSource;
import net.minecraft.world.level.levelgen.PositionalRandomFactory;
import net.minecraft.world.level.levelgen.WorldgenRandom;
import net.minecraft.world.level.levelgen.XoroshiroRandomSource;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

public class JsonManager {
    // Key = Density Function data path
    public final static Map<String, OperationArg> densityFunctions = new HashMap<>();
    public final Map<String, NoiseSettings> worldTypes = new HashMap<>();
    public final Map<String, NoiseParameter> noiseParams = new HashMap<>();
    private final static GsonBuilder defaultGsonBuilder = new GsonBuilder()
            .registerTypeAdapter(OperationArg.class, new OperationDeserializer())
            .registerTypeAdapter(Spline.SplineArg.class, new Spline.SplineDeserializer())
            .setFieldNamingPolicy(FieldNamingPolicy.LOWER_CASE_WITH_UNDERSCORES);
    private final static Gson gson = defaultGsonBuilder
            .registerTypeAdapter(OperationArg.class, new OperationSerializer())
            .create();

    public JsonManager(Class<?> anchorClass) throws Exception {
        List<String> jsonFilePaths = JsonManager.findJsonFiles(anchorClass);
        for (String f : jsonFilePaths) {
            if (f.startsWith("data/minecraft/worldgen/density_function/")) {
                String json;
                try (InputStream in = Main.class.getResourceAsStream("/" + f)) {
                    if (in == null) throw new RuntimeException("Not found");
                    json = new String(in.readAllBytes(), StandardCharsets.UTF_8);
                } catch (RuntimeException | IOException e) {
                    throw new RuntimeException(e);
                }
                densityFunctions.put(
                        "minecraft:" + f.replaceFirst("data/minecraft/worldgen/density_function/", "")
                                .replaceFirst(".json", ""),
                        gson.fromJson(json, OperationArg.class)
                );
            } else if (f.startsWith("data/minecraft/worldgen/noise_settings")) {
                String json;
                try (InputStream in = Main.class.getResourceAsStream("/" + f)) {
                    if (in == null) throw new RuntimeException("Not found");
                    json = new String(in.readAllBytes(), StandardCharsets.UTF_8);
                } catch (RuntimeException | IOException e) {
                    throw new RuntimeException(e);
                }
                worldTypes.put(
                        f.replaceFirst("data/minecraft/worldgen/noise_settings/", "")
                                .replaceFirst(".json", ""),
                        gson.fromJson(json, NoiseSettings.class)
                );
            } else if (f.startsWith("data/minecraft/worldgen/noise/")) {
                String json;
                try (InputStream in = Main.class.getResourceAsStream("/" + f)) {
                    if (in == null) throw new RuntimeException("Not found");
                    json = new String(in.readAllBytes(), StandardCharsets.UTF_8);
                } catch (RuntimeException | IOException e) {
                    throw new RuntimeException(e);
                }
                noiseParams.put(
                        "minecraft:" + f.replaceFirst("data/minecraft/worldgen/noise/", "")
                                .replaceFirst(".json", ""),
                        new Gson().fromJson(json, NoiseParameter.class)
                );
            }
        }
        // Ensure noises for all density functions are generated
        for (Map.Entry<String, NoiseSettings> e : worldTypes.entrySet()) {
            e.getValue().constructDensityFunctions(noiseParams, e.getKey());
        }
    }

    private static List<String> findJsonFiles(Class<?> anchorClass) throws Exception {
        List<String> filePaths = new ArrayList<>();

        URL jarPath = anchorClass.getProtectionDomain()
                .getCodeSource().getLocation();
        File jarFile = new File(jarPath.toURI());

        try (JarFile jar = new JarFile(jarFile)) {
            Enumeration<JarEntry> entries = jar.entries();

            while (entries.hasMoreElements()) {
                JarEntry entry = entries.nextElement();
                String name = entry.getName();
                if (!entry.isDirectory() && name.endsWith(".json")) {
                    filePaths.add(name);
                }
            }
        }
        return filePaths;
    }

    public static class NoiseSettings {
        public boolean legacyRandomSource;
        public HashMap<String, OperationArg> noiseRouter;
        public HashMap<String, String> jsonDensityFunctions;

        public void constructDensityFunctions(Map<String, NoiseParameter> noiseParams, String world) {
            this.jsonDensityFunctions = new HashMap<>();
            PositionalRandomFactory rand = legacyRandomSource
                    ? WorldgenRandom.Algorithm.LEGACY.newInstance(LudiGen.seed.get()).forkPositional()
                    : WorldgenRandom.Algorithm.XOROSHIRO.newInstance(LudiGen.seed.get()).forkPositional();
            for (Map.Entry<String, OperationArg> e: this.noiseRouter.entrySet()) {
                String dfIdentifier = world + ":" + e.getKey();
                String json = new GsonBuilder()
                        .registerTypeAdapter(ExternalArg.class, new ExternalArgSerializer())
                        .registerTypeAdapter(Noise.class, new NoiseSerializer(rand, noiseParams, dfIdentifier))
                        .registerTypeAdapter(ShiftedNoise.class, new ShiftedNoiseSerializer(rand, noiseParams, dfIdentifier))
                        .registerTypeAdapter(ShiftA.class, new ShiftASerializer(rand, noiseParams, dfIdentifier))
                        .registerTypeAdapter(ShiftB.class, new ShiftBSerializer(rand, noiseParams, dfIdentifier))
                        .registerTypeAdapter(CacheOnce.class, new CacheOnceSerializer())
                        .registerTypeAdapter(FlatCache.class, new FlatCacheSerializer())
                        .registerTypeAdapter(Cache2D.class, new Cache2DSerializer())
                        .registerTypeAdapter(ConstantArg.class, new ConstantArgSerializer())
                        .setFieldNamingPolicy(FieldNamingPolicy.LOWER_CASE_WITH_UNDERSCORES)
                        .create()
                        .toJson(e.getValue());
                jsonDensityFunctions.put(e.getKey(), json);
            }
        }
    }

    public static class NoiseParameter {
        public int firstOctave;
        public double[] amplitudes;
    }
}
