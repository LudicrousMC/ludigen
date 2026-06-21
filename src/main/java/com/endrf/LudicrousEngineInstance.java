package com.endrf;

import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class LudicrousEngineInstance {
    private final long ptr;
    private final LudicrousEngine engine;

    public LudicrousEngineInstance(long ptr) {
        System.out.println(ptr);
        this.ptr = ptr;
        this.engine = new LudicrousEngine();
    }

    public void registerShader(String shader) throws IOException {
        byte[] shaderBytes;
        try (InputStream in = LudicrousEngine.class.getResourceAsStream("/compute/" + shader + ".spv")) {
            if (in == null) throw new RuntimeException("Missing Compute Shader: " + shader + ".spv");
            shaderBytes = in.readAllBytes();
        }
        ByteBuffer spirv = ByteBuffer.allocateDirect(shaderBytes.length);
        spirv.order(ByteOrder.nativeOrder());
        spirv.put(shaderBytes);
        spirv.flip();
        engine.loadShader(this.ptr, spirv);
    }
}
