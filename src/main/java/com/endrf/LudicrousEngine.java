package com.endrf;

import io.netty.buffer.ByteBuf;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

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

    public static void registerShader(String shader) throws IOException {
        byte[] shaderBytes;
        try (InputStream in = LudicrousEngine.class.getResourceAsStream("/compute/" + shader + ".spv")) {
            if (in == null) throw new RuntimeException("Missing Compute Shader: " + shader + ".spv");
            shaderBytes = in.readAllBytes();
        }
        ByteBuffer spirv = ByteBuffer.allocateDirect(shaderBytes.length);
        spirv.order(ByteOrder.nativeOrder());
        spirv.put(shaderBytes);
        spirv.flip();
        addShader(spirv);
    }

    private static native void addShader(ByteBuffer spirv);
    public static native void test();
}
