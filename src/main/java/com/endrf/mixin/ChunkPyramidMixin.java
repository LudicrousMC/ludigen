package com.endrf.mixin;

import net.minecraft.server.level.GenerationChunkHolder;
import net.minecraft.util.StaticCache2D;
import net.minecraft.world.level.chunk.ChunkAccess;
import net.minecraft.world.level.chunk.status.*;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

import java.util.concurrent.CompletableFuture;

@Mixin(ChunkPyramid.class)
public class ChunkPyramidMixin {
    @Shadow
    @Final
    @Mutable
    private static ChunkPyramid GENERATION_PYRAMID;

    @Inject(method = "<clinit>", at = @At("TAIL"))
    private static void changeValue(CallbackInfo ci) {
        GENERATION_PYRAMID = new ChunkPyramid.Builder()
                .step(ChunkStatus.EMPTY, s -> s)
                .step(ChunkStatus.STRUCTURE_STARTS, s -> s.setTask((WorldGenContext context, ChunkStep step, StaticCache2D<GenerationChunkHolder> chunks, ChunkAccess chunk) -> {
                    //long startTime = System.nanoTime();
                    CompletableFuture<ChunkAccess> chunkFut = ChunkStatusTasks.generateStructureStarts(context, step, chunks, chunk);
                    chunkFut.join();
                    //long endTime = System.nanoTime();
                    //System.out.println(endTime - startTime);
                    return chunkFut;
                }))
                .step(ChunkStatus.STRUCTURE_REFERENCES, s -> s.addRequirement(ChunkStatus.STRUCTURE_STARTS, 8).setTask((WorldGenContext context, ChunkStep step, StaticCache2D<GenerationChunkHolder> chunks, ChunkAccess chunk) -> {
                    //long startTime = System.nanoTime();
                    CompletableFuture<ChunkAccess> chunkFut = ChunkStatusTasks.generateStructureReferences(context, step, chunks, chunk);
                    chunkFut.join();
                    //long endTime = System.nanoTime();
                    //System.out.println(endTime - startTime);
                    return chunkFut;
                }))
                .step(ChunkStatus.BIOMES, s -> s.addRequirement(ChunkStatus.STRUCTURE_STARTS, 8).setTask((WorldGenContext context, ChunkStep step, StaticCache2D<GenerationChunkHolder> chunks, ChunkAccess chunk) -> {
                    //long startTime = System.nanoTime();
                    CompletableFuture<ChunkAccess> chunkFut = ChunkStatusTasks.generateBiomes(context, step, chunks, chunk);
                    chunkFut.join();
                    //long endTime = System.nanoTime();
                    //System.out.println(endTime - startTime);
                    return chunkFut;
                }))
                .step(
                        ChunkStatus.NOISE,
                        s -> s.addRequirement(ChunkStatus.STRUCTURE_STARTS, 8)
                                .addRequirement(ChunkStatus.BIOMES, 1)
                                .blockStateWriteRadius(0)
                                .setTask((WorldGenContext context, ChunkStep step, StaticCache2D<GenerationChunkHolder> chunks, ChunkAccess chunk) -> {
                                    //long startTime = System.nanoTime();
                                    CompletableFuture<ChunkAccess> chunkFut = ChunkStatusTasks.generateNoise(context, step, chunks, chunk);
                                    chunkFut.join();
                                    //long endTime = System.nanoTime();
                                    //System.out.println(endTime - startTime);
                                    return chunkFut;
                                })
                )
                .step(
                        ChunkStatus.SURFACE,
                        s -> s.addRequirement(ChunkStatus.STRUCTURE_STARTS, 8)
                                .addRequirement(ChunkStatus.BIOMES, 1)
                                .blockStateWriteRadius(0)
                                .setTask((WorldGenContext context, ChunkStep step, StaticCache2D<GenerationChunkHolder> chunks, ChunkAccess chunk) -> {
                                    //long startTime = System.nanoTime();
                                    CompletableFuture<ChunkAccess> chunkFut = ChunkStatusTasks.generateSurface(context, step, chunks, chunk);
                                    chunkFut.join();
                                    //long endTime = System.nanoTime();
                                    //System.out.println(endTime - startTime);
                                    return chunkFut;
                                })
                )
                .step(ChunkStatus.CARVERS, s -> s.addRequirement(ChunkStatus.STRUCTURE_STARTS, 8).blockStateWriteRadius(0).setTask((WorldGenContext context, ChunkStep step, StaticCache2D<GenerationChunkHolder> chunks, ChunkAccess chunk) -> {
                    //long startTime = System.nanoTime();
                    CompletableFuture<ChunkAccess> chunkFut = ChunkStatusTasks.generateCarvers(context, step, chunks, chunk);
                    chunkFut.join();
                    //long endTime = System.nanoTime();
                    //System.out.println(endTime - startTime);
                    return chunkFut;
                }))
                .step(
                        ChunkStatus.FEATURES,
                        s -> s.addRequirement(ChunkStatus.STRUCTURE_STARTS, 8)
                                .addRequirement(ChunkStatus.CARVERS, 1)
                                .blockStateWriteRadius(1)
                                .setTask((WorldGenContext context, ChunkStep step, StaticCache2D<GenerationChunkHolder> chunks, ChunkAccess chunk) -> {
                                    //long startTime = System.nanoTime();
                                    CompletableFuture<ChunkAccess> chunkFut = ChunkStatusTasks.generateFeatures(context, step, chunks, chunk);
                                    chunkFut.join();
                                    //long endTime = System.nanoTime();
                                    //System.out.println(endTime - startTime);
                                    return chunkFut;
                                })
                )
                .step(ChunkStatus.INITIALIZE_LIGHT, s -> s.setTask((WorldGenContext context, ChunkStep step, StaticCache2D<GenerationChunkHolder> chunks, ChunkAccess chunk) -> {
                    //long startTime = System.nanoTime();
                    CompletableFuture<ChunkAccess> chunkFut = ChunkStatusTasks.initializeLight(context, step, chunks, chunk);
                    chunkFut.join();
                    //long endTime = System.nanoTime();
                    //System.out.println(endTime - startTime);
                    return chunkFut;
                }))
                .step(ChunkStatus.LIGHT, s -> s.addRequirement(ChunkStatus.INITIALIZE_LIGHT, 1).setTask((WorldGenContext context, ChunkStep step, StaticCache2D<GenerationChunkHolder> chunks, ChunkAccess chunk) -> {
                    //long startTime = System.nanoTime();
                    CompletableFuture<ChunkAccess> chunkFut = ChunkStatusTasks.light(context, step, chunks, chunk);
                    chunkFut.join();
                    //long endTime = System.nanoTime();
                    //System.out.println(endTime - startTime);
                    return chunkFut;
                }))
                .step(ChunkStatus.SPAWN, s -> s.addRequirement(ChunkStatus.BIOMES, 1).setTask((WorldGenContext context, ChunkStep step, StaticCache2D<GenerationChunkHolder> chunks, ChunkAccess chunk) -> {
                    //long startTime = System.nanoTime();
                    CompletableFuture<ChunkAccess> chunkFut = ChunkStatusTasks.generateSpawn(context, step, chunks, chunk);
                    chunkFut.join();
                    //long endTime = System.nanoTime();
                    //System.out.println(endTime - startTime);
                    return chunkFut;
                }))
                .step(ChunkStatus.FULL, s -> s.setTask((WorldGenContext context, ChunkStep step, StaticCache2D<GenerationChunkHolder> chunks, ChunkAccess chunk) -> {
                    //long startTime = System.nanoTime();
                    CompletableFuture<ChunkAccess> chunkFut = ChunkStatusTasks.full(context, step, chunks, chunk);
                    chunkFut.join();
                    //long endTime = System.nanoTime();
                    //System.out.println(endTime - startTime);
                    return chunkFut;
                }))
                .build();
    }
}
