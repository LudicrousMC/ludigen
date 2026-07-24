package com.endrf;

import io.netty.buffer.ByteBuf;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;

public class LudicrousEngine {
    static {
        loadLib("libludicrous_engine");
    }

    private static void loadLib(String libFile) {
        String libPath = "/natives/" + libFile + ".so";
        try (InputStream in = LudicrousEngine.class.getResourceAsStream(libPath)) {
            if (in == null) {
                throw new RuntimeException("Missing Ludicrous Engine Library: " + libFile);
            }

            Path tmp = Files.createTempFile(libFile, ".so");
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

    native void loadShader(long instance, ByteBuffer spirv);

    native void setDensityFunction(long instance, String fnJson);

    native void setNoiseStates(long instance, ArrayList<NoiseStates.NormalNoiseState> noiseStates, ArrayList<NoiseStates.ImprovedNoiseState> noiseLevelStates);

    native void evalDensityFunction(long instance);
}
