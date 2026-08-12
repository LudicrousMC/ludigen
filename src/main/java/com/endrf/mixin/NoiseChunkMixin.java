package com.endrf.mixin;

import com.endrf.LudiGen;
import com.google.common.collect.Lists;
import net.minecraft.core.QuartPos;
import net.minecraft.core.SectionPos;
import net.minecraft.util.Mth;
import net.minecraft.world.level.ChunkPos;
import net.minecraft.world.level.block.Blocks;
import net.minecraft.world.level.block.state.BlockState;
import net.minecraft.world.level.levelgen.*;
import net.minecraft.world.level.levelgen.blending.Blender;
import net.minecraft.world.level.levelgen.material.MaterialRuleList;
import org.jetbrains.annotations.Nullable;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Mutable;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

import java.util.ArrayList;
import java.util.List;

@Mixin(NoiseChunk.class)
public abstract class NoiseChunkMixin {
    @Mutable
    @Final
    @Shadow
    private int cellWidth;

    @Mutable
    @Final
    @Shadow
    private int cellHeight;

    @Mutable
    @Final
    @Shadow
    private int cellCountXZ;

    @Mutable
    @Final
    @Shadow
    private int cellCountY;

    @Mutable
    @Final
    @Shadow
    private int cellNoiseMinY;

    @Mutable
    @Final
    @Shadow
    private int firstCellX;

    @Mutable
    @Final
    @Shadow
    private int firstCellZ;

    @Mutable
    @Final
    @Shadow
    private List<NoiseChunk.NoiseInterpolator> interpolators;

    @Mutable
    @Final
    @Shadow
    private List<NoiseChunk.FlatCache> cellCaches;

    @Mutable
    @Final
    @Shadow
    private int firstNoiseX;

    @Mutable
    @Final
    @Shadow
    private int firstNoiseZ;

    @Mutable
    @Final
    @Shadow
    private int noiseSizeXZ;

    @Mutable
    @Final
    @Shadow
    private Blender blender;

    @Mutable
    @Final
    @Shadow
    private DensityFunctions.BeardifierOrMarker beardifier;

    @Mutable
    @Final
    @Shadow
    private NoiseChunk.@Nullable FlatCache blendAlpha;

    @Mutable
    @Final
    @Shadow
    private NoiseChunk.@Nullable FlatCache blendOffset;

    @Shadow
    public abstract DensityFunction wrap(DensityFunction function);

    @Mutable
    @Final
    @Shadow
    private DensityFunction preliminarySurfaceLevel;

    @Mutable
    @Final
    @Shadow
    private Aquifer aquifer;

    @Mutable
    @Final
    @Shadow
    private DensityFunction fullNoiseDensity;

    @Mutable
    @Final
    @Shadow
    private NoiseChunk.BlockStateFiller blockStateRule;

    @Inject(method = "<init>", at = @At("TAIL"))
    void NoiseChunk(int cellCountXZ, RandomState randomState, int chunkMinBlockX, int chunkMinBlockZ, NoiseSettings noiseSettings, DensityFunctions.BeardifierOrMarker beardifier, NoiseGeneratorSettings settings, Aquifer.FluidPicker globalFluidPicker, Blender blender, CallbackInfo ci) {
        this.cellWidth = noiseSettings.getCellWidth();
        this.cellHeight = noiseSettings.getCellHeight();
        this.cellCountXZ = cellCountXZ;
        this.cellCountY = Mth.floorDiv(noiseSettings.height(), this.cellHeight);
        this.cellNoiseMinY = Mth.floorDiv(noiseSettings.minY(), this.cellHeight);
        this.firstCellX = Math.floorDiv(chunkMinBlockX, this.cellWidth);
        this.firstCellZ = Math.floorDiv(chunkMinBlockZ, this.cellWidth);
        this.interpolators = Lists.newArrayList();
        this.cellCaches = Lists.newArrayList();
        this.firstNoiseX = QuartPos.fromBlock(chunkMinBlockX);
        this.firstNoiseZ = QuartPos.fromBlock(chunkMinBlockZ);
        this.noiseSizeXZ = QuartPos.fromBlock(cellCountXZ * this.cellWidth);
        this.blender = Blender.empty();
        this.beardifier = beardifier;
        if (!blender.isEmpty()) {
            this.blendAlpha = null;//new NoiseChunk.FlatCache(new NoiseChunk.BlendAlpha(), false);
            this.blendOffset = null;//new NoiseChunk.FlatCache(new NoiseChunk.BlendOffset(), false);

            for (int x = 0; x <= this.noiseSizeXZ; x++) {
                int quartX = this.firstNoiseX + x;
                int blockX = QuartPos.toBlock(quartX);

                for (int z = 0; z <= this.noiseSizeXZ; z++) {
                    int quartZ = this.firstNoiseZ + z;
                    int blockZ = QuartPos.toBlock(quartZ);
                    Blender.BlendingOutput blendingOutput = blender.blendOffsetAndFactor(blockX, blockZ);
                    this.blendAlpha.values[x + z * this.blendAlpha.sizeXZ] = blendingOutput.alpha();
                    this.blendOffset.values[x + z * this.blendOffset.sizeXZ] = blendingOutput.blendingOffset();
                }
            }
        } else {
            this.blendAlpha = null;
            this.blendOffset = null;
        }


        NoiseRouter router = randomState.router();
        NoiseRouter wrappedRouter = router.mapAll(this::wrap);
        this.preliminarySurfaceLevel = wrappedRouter.preliminarySurfaceLevel();
        if (!settings.isAquifersEnabled()) {
            this.aquifer = Aquifer.createDisabled(globalFluidPicker);
        } else {
            int chunkX = SectionPos.blockToSectionCoord(chunkMinBlockX);
            int chunkZ = SectionPos.blockToSectionCoord(chunkMinBlockZ);
            this.aquifer = Aquifer.create(
                    (NoiseChunk) (Object) this, new ChunkPos(chunkX, chunkZ), wrappedRouter, randomState.aquiferRandom(), noiseSettings.minY(), noiseSettings.height(), globalFluidPicker
            );
        }

        List<NoiseChunk.BlockStateFiller> builder = new ArrayList<>();
        DensityFunction fullNoiseValue = DensityFunctions.cacheAllInCell(DensityFunctions.add(wrappedRouter.finalDensity(), DensityFunctions.BeardifierMarker.INSTANCE))
                .mapAll(this::wrap);
        this.fullNoiseDensity = fullNoiseValue;
        builder.add((NoiseChunk.BlockStateFiller)(context) -> {
            //double density = fullNoiseValue.compute(context);
            //double density;
            //if (context.blockY() % 8 == 0 && context.blockX() % 4 == 0 && context.blockZ() % 4 == 0) {
            double density = LudiGen.instance.evalDensityFunction(context.blockX(), context.blockY(), context.blockZ());
            /*} else {
                density = -0.5;
            }*/
            BlockState result = this.aquifer.computeSubstance(context, density);
            return result;
            //return Blocks.AIR.defaultBlockState();
        });
        if (settings.oreVeinsEnabled()) {
            builder.add(OreVeinifier.create(wrappedRouter.veinToggle(), wrappedRouter.veinRidged(), wrappedRouter.veinGap(), randomState.oreRandom()));
        }

        this.blockStateRule = new MaterialRuleList((NoiseChunk.BlockStateFiller[])builder.toArray(new NoiseChunk.BlockStateFiller[0]));
    }
}
