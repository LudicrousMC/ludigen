package com.endrf;

import com.endrf.operations.*;
import com.google.gson.*;
import net.minecraft.server.Main;

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
    // Key = Dimension/type
    public final static Map<String, OperationArg> finalDensityFunctions = new HashMap<>();
    private final GsonBuilder defaultGsonBuilder = new GsonBuilder()
            .registerTypeAdapter(OperationArg.class, new OperationDeserializer())
            .registerTypeAdapter(Spline.SplineArg.class, new Spline.SplineDeserializer())
            .registerTypeAdapter(OperationArg.class, new OperationSerializer())
            .setFieldNamingPolicy(FieldNamingPolicy.LOWER_CASE_WITH_UNDERSCORES);
    private final Gson gson = defaultGsonBuilder.create();
    private final Gson gsonConstruct = defaultGsonBuilder
            .registerTypeAdapter(OperationArg.class, new ConstructionSerializer())
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
                JsonObject obj = JsonParser.parseString(json).getAsJsonObject();
                finalDensityFunctions.put(
                        f.replaceFirst("data/minecraft/worldgen/noise_settings/", "")
                                .replaceFirst(".json", ""),
                        gson.fromJson(obj.get("noise_router")
                                .getAsJsonObject().get("final_density").getAsJsonObject(), OperationArg.class)
                );
            }
        }
        String finalDensity = gsonConstruct.toJson(finalDensityFunctions.get("overworld"));
        System.out.println(finalDensity.length());
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
}
