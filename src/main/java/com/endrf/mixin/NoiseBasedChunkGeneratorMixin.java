package com.endrf.mixin;

import com.endrf.LudiGen;
import com.google.common.collect.Sets;
import net.minecraft.core.Holder;
import net.minecraft.util.Mth;
import net.minecraft.util.Util;
import net.minecraft.world.level.ChunkPos;
import net.minecraft.world.level.StructureManager;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.chunk.ChunkAccess;
import net.minecraft.world.level.chunk.LevelChunkSection;
import net.minecraft.world.level.levelgen.*;
import net.minecraft.world.level.levelgen.blending.Blender;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

import java.util.ArrayList;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.function.Supplier;

@Mixin(NoiseBasedChunkGenerator.class)
public abstract class NoiseBasedChunkGeneratorMixin {

    @Shadow
    @Final
    private Holder<NoiseGeneratorSettings> settings;

    @Shadow
    protected abstract NoiseChunk createNoiseChunk(ChunkAccess chunk, StructureManager structureManager, Blender blender, RandomState randomState);

    @Shadow
    public abstract int getMinY();

    @Shadow
    @Final
    private Supplier<Aquifer.FluidPicker> globalFluidPicker;

    @Shadow
    @Final
    private static BlockState AIR;

    @Inject(method = "fillFromNoise", at = @At("HEAD"), cancellable = true)
    void onFillFromNoise(
            Blender blender,
            RandomState randomState,
            StructureManager structureManager,
            ChunkAccess centerChunk,
            CallbackInfoReturnable<CompletableFuture<ChunkAccess>> cir
    ) {
        cir.cancel();
        NoiseSettings noiseSettings = this.settings.value().noiseSettings().clampToHeightAccessor(centerChunk.getHeightAccessorForGeneration());
        int cellCountY = Mth.floorDiv(noiseSettings.height(), noiseSettings.getCellHeight());
        if (cellCountY <= 0) {
            cir.setReturnValue(CompletableFuture.completedFuture(centerChunk));
            return;
        }
        cir.setReturnValue(CompletableFuture.supplyAsync(() -> {
            int minY = noiseSettings.minY();
            int topIndex = centerChunk.getSectionIndex(cellCountY * noiseSettings.getCellHeight() - 1 + minY);
            int bottomIndex = centerChunk.getSectionIndex(minY);
            ArrayList<LevelChunkSection> sections = new ArrayList<>();
            for (int sectionIndex = topIndex; sectionIndex >= bottomIndex; sectionIndex--) {
                LevelChunkSection section = centerChunk.getSection(sectionIndex);
                section.acquire();
                sections.add(section);
            }

            ChunkPos pos = centerChunk.getPos();
            Aquifer aquifer;
            if (!this.settings.value().isAquifersEnabled()) {
                aquifer = Aquifer.createDisabled(this.globalFluidPicker.get());
            } else {
                NoiseChunk noiseChunk = centerChunk.getOrCreateNoiseChunk(chunk -> this.createNoiseChunk(centerChunk, structureManager, blender, randomState));
                aquifer = Aquifer.create(noiseChunk, pos, this.settings.value().noiseRouter(), randomState.aquiferRandom(), noiseSettings.minY(), noiseSettings.height(), globalFluidPicker.get());
            }
            NoiseChunk.BlockStateFiller veinifier = null;
            if (this.settings.value().oreVeinsEnabled()) {
                veinifier = OreVeinifier.create(this.settings.value().noiseRouter().veinToggle(), this.settings.value().noiseRouter().veinRidged(), this.settings.value().noiseRouter().veinGap(), randomState.oreRandom());
            }

            double[] densities = LudiGen.instance.fillNoise(minY, noiseSettings.height(), noiseSettings.getCellWidth(), noiseSettings.getCellHeight(), pos.getMinBlockX(), pos.getMinBlockZ());
            Heightmap oceanFloor = centerChunk.getOrCreateHeightmapUnprimed(Heightmap.Types.OCEAN_FLOOR_WG);
            Heightmap worldSurface = centerChunk.getOrCreateHeightmapUnprimed(Heightmap.Types.WORLD_SURFACE_WG);

            int yInChunk = noiseSettings.height() + minY;
            int yIndex = noiseSettings.height();
            for (LevelChunkSection section : sections) {
                for (int yInSection = 15; yInSection >= 0; yInSection--, yInChunk--, yIndex--) {
                    for (int zInChunk = 0; zInChunk < 16; zInChunk++) {
                        for (int xInChunk = 0; xInChunk < 16; xInChunk++) {
                            int finalXInChunk = xInChunk;
                            int finalYInChunk = yInChunk;
                            int finalZInChunk = zInChunk;
                            DensityFunction.FunctionContext context = new DensityFunction.FunctionContext() {
                                @Override
                                public int blockX() {
                                    return pos.getMinBlockX() + finalXInChunk;
                                }

                                @Override
                                public int blockY() {
                                    return finalYInChunk;
                                }

                                @Override
                                public int blockZ() {
                                    return pos.getMinBlockZ() + finalZInChunk;
                                }
                            };
                            //double density = LudiGen.instance.evalDensityFunction(pos.getMinBlockX() + xInChunk, yInChunk, pos.getMinBlockZ() + zInChunk);
                            double density = densities[(yIndex * 256) + (zInChunk * 16) + xInChunk];
                            BlockState state = aquifer.computeSubstance(context, density);
                            if (state == null && veinifier != null) {
                                state = veinifier.calculate(context);
                            }

                            if (state == null) {
                                state = this.settings.value().defaultBlock();
                            }

                            if (state != AIR) {
                                section.setBlockState(xInChunk, yInSection, zInChunk, state, false);
                                oceanFloor.update(xInChunk, yInChunk, zInChunk, state);
                                worldSurface.update(xInChunk, yInChunk, zInChunk, state);
                            }
                        }
                    }
                }
            }

            for (LevelChunkSection s : sections) {
                s.release();
            }
            return centerChunk;
        }, Util.backgroundExecutor().forName("wgen_fill_noise")));
    }


}
