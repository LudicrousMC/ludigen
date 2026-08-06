package com.endrf.mixin;

import net.minecraft.world.level.levelgen.DensityFunction;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

import java.util.Arrays;

@Mixin(targets = "net.minecraft.world.level.levelgen.DensityFunctions$Ap2")
public class Ap2Mixin {
    @Inject(method = "compute", at = @At("HEAD"), cancellable = true)
    private void onCompute(DensityFunction.FunctionContext context, CallbackInfoReturnable<Double> cir) {
        System.out.println("dense func compute call");
        cir.setReturnValue(0.0);
        cir.cancel();
    }

    @Inject(method = "fillArray", at = @At("HEAD"), cancellable = true)
    private void onFillArray(double[] output, DensityFunction.ContextProvider contextProvider, CallbackInfo ci) {
        System.out.println("dense func fillArray call");
        Arrays.fill(output, 0.0);
        ci.cancel();
    }
}
