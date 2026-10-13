package com.endrf;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;

public class LudicrousEngine {
    static {
        loadLib("libludicrous_engine");
    }

    private static void loadLib(String libFile) {
        String os = System.getProperty("os.name").toLowerCase();
        String fileExt;
        if (os.contains("win")) {
            fileExt = ".dll";
        } else if (os.contains("mac")) {
            throw new RuntimeException("LudiGen does not currently support MacOS");
            //fileExt = ".dylib";
        } else if (os.contains("nix") || os.contains("nux")) {
            fileExt = ".so";
        } else {
            throw new RuntimeException("No valid Ludicrous Engine Library for target OS: " + os);
        }
        String libPath = "/natives/" + libFile + fileExt;
        try (InputStream in = LudicrousEngine.class.getResourceAsStream(libPath)) {
            if (in == null) {
                throw new RuntimeException("Missing Ludicrous Engine Library: " + libFile);
            }

            Path tmp = Files.createTempFile(libFile, fileExt);
            tmp.toFile().deleteOnExit();

            Files.copy(in, tmp, StandardCopyOption.REPLACE_EXISTING);
            System.load(tmp.toAbsolutePath().toString());
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private static native long newInstance();
    public static LudicrousEngineInstance createInstance() {
        return new LudicrousEngineInstance(newInstance());
    }

    static native void loadFinalDensityShader(
            long instance,
            int scale,
            int minY,
            int height,
            int noiseX,
            int noiseY,
            String json,
            ArrayList<NoiseStates.NormalNoiseState> noiseStates,
            ArrayList<NoiseStates.ImprovedNoiseState> noiseLevels,
            String dimension
    );

    static native double evalDensityFunction(long instance, int x, int y, int z);

    static native double[] fillNoise(long instance, int minY, int height, int cellWidth, int cellHeight, int x, int z);

    static native ByteBuffer fillNoiseAccel(long instance, int fdIndex, int x, int z, int[] retType);

    static native void releaseDensities(long instance, ByteBuffer buf, int cachedChunkIndex);
}
