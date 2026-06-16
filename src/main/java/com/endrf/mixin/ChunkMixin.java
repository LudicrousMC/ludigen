package com.endrf.mixin;

import net.minecraft.server.level.*;
import net.minecraft.world.level.ChunkPos;
import net.minecraft.world.level.TicketStorage;
import net.minecraft.world.level.chunk.status.ChunkStatus;
import org.spongepowered.asm.mixin.Final;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.Shadow;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfoReturnable;

import java.util.concurrent.CompletableFuture;

@Mixin(ChunkMap.class)
public abstract class ChunkMixin {
    @Shadow
    @Final
    private ServerLevel level;

    @Inject(method = "applyChunkTrackingView", at = @At("HEAD"), cancellable = true)
    private void onApplyChunkTrackingView(ServerPlayer player, ChunkTrackingView next, CallbackInfo ci) {
        if (player.level() == this.level) {
            player.setChunkTrackingView(next);
        }
        ci.cancel();
        System.out.println("new chunk load");
    }
}

/*@Mixin(ServerChunkCache.class)
class ChunkCacheMixin {
    @Inject(method = "addTicketAndLoadWithRadius", at = @At("HEAD"), cancellable = true)
    public void onAddTicketAndLoadWithRadius(TicketType type, ChunkPos pos, int radius, CallbackInfoReturnable<CompletableFuture<?>> cir) {
        cir.setReturnValue(CompletableFuture.completedFuture(null));
        cir.cancel();
    }
}

@Mixin(TicketStorage.class)
class TicketMixin {
    @Inject(method = "addTicket(JLnet/minecraft/server/level/Ticket;)Z", at = @At("HEAD"), cancellable = true)
    private void onAddTicket(long key, Ticket ticket, CallbackInfoReturnable<Boolean> cir) {
        cir.cancel();
        cir.setReturnValue(true);
    }
}*/

/*@Mixin(ServerChunkCache.class)
public class ChunkMixin {
    @Inject(method = "addTicket", at = @At("HEAD"), cancellable = true)
    private void onAddTicket(Ticket ticket, ChunkPos pos, CallbackInfo ci) {
        if (ticket.getType() == TicketType.PLAYER_SPAWN || ticket.getType() == TicketType.PLAYER_LOADING || ticket.getType() == TicketType.PLAYER_SIMULATION || ticket.getType() == TicketType.FORCED) {
            ci.cancel();
        }
    }

    *//*@Inject(method = "getChunk", at = @At("HEAD"), cancellable = true)
    private void onGetChunk(int x, int z, ChunkStatus targetStatus, boolean loadOrGenerate, CallbackInfoReturnable<ChunkHolder> cir) {
        if (loadOrGenerate) {
            cir.setReturnValue(null);
            cir.cancel();
        }
    }*//*

    @Inject(method = "move", at = @At("HEAD"), cancellable = true)
    private void onMove(ServerPlayer player, CallbackInfo ci) {
        ci.cancel();
    }

    *//*@Inject(method = "hasChunk", at = @At("HEAD"), cancellable = true)
    private void onHasChunk(int x, int z, CallbackInfoReturnable<Boolean> cir) {
        cir.setReturnValue(false);
    }*//*
}*/
