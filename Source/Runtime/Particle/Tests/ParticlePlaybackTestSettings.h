#pragma once

#include "Particle/ParticleTypes.h"
#include "Particle/ParticleModule.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Particle/Modules/ParticleModuleLifeTime.h"
#include "Particle/Modules/ParticleModuleVelocity.h"
#include "Particle/Modules/ParticleModuleLocation.h"
#include "Particle/Modules/ParticleModuleRotation.h"
#include "Particle/Modules/ParticleModuleRotationRate.h"
#include "Particle/Modules/ParticleModuleSize.h"
#include "Particle/Modules/ParticleModuleSpawn.h"
#include "Particle/Modules/ParticleModuleSizeOfLifeTime.h"
#include "Particle/Modules/ParticleModuleColor.h"
#include "Particle/Modules/ParticleModuleColorOverLife.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Component/FXSystemComponent.h"
#include "Component/ParticleSystemComponent.h"
#include "Asset/AssetManager.h"
#include "Render/Material.h"
#include "Core/EngineLog.h"
#include <memory>

// Temporary hard-coded playback settings, owned by the test actor.
// All simulation remains in the trainee's existing modules and component.
struct FParticlePlaybackEmitterTestSettings
{
    std::unique_ptr<UParticleEmitter> Emitter;
    std::unique_ptr<UParticleLODLevel> LOD;
    std::unique_ptr<UParticleModuleRequired> Required;
    std::unique_ptr<UParticleModuleSpawn> Spawn;
    std::unique_ptr<UParticleModuleLifeTime> Lifetime;
    std::unique_ptr<UParticleModuleLocation> Location;
    std::unique_ptr<UParticleModuleVelocity> Velocity;
    std::unique_ptr<UParticleModuleRotation> Rotation;
    std::unique_ptr<UParticleModuleRotationRate> RotationRate;
    std::unique_ptr<UParticleModuleSize> Size;
    std::unique_ptr<UParticleModuleSizeOfLifeTime> SizeOverLife;
    std::unique_ptr<UParticleModuleColor> Color;
    std::unique_ptr<UParticleModuleColorOverLife> ColorOverLife;

    FParticlePlaybackEmitterTestSettings(UParticleSystem* System, bool bLocal)
        : Emitter(NewObject<UParticleEmitter>(System))
        , LOD(NewObject<UParticleLODLevel>(Emitter.get()))
        , Required(NewObject<UParticleModuleRequired>(Emitter.get()))
        , Spawn(NewObject<UParticleModuleSpawn>(Emitter.get()))
        , Lifetime(NewObject<UParticleModuleLifeTime>(Emitter.get()))
        , Location(NewObject<UParticleModuleLocation>(Emitter.get()))
        , Velocity(NewObject<UParticleModuleVelocity>(Emitter.get()))
        , Rotation(NewObject<UParticleModuleRotation>(Emitter.get()))
        , RotationRate(NewObject<UParticleModuleRotationRate>(Emitter.get()))
        , Size(NewObject<UParticleModuleSize>(Emitter.get()))
        , SizeOverLife(NewObject<UParticleModuleSizeOfLifeTime>(Emitter.get()))
        , Color(NewObject<UParticleModuleColor>(Emitter.get()))
        , ColorOverLife(NewObject<UParticleModuleColorOverLife>(Emitter.get()))
    {
        // Existing defaults: 10 particles/s, 5s lifetime, origin, upward velocity,
        // unit initial size and scale 1 -> 2. No new simulation behavior here.
        const FLinearColor Tint = bLocal
            ? FLinearColor(0.0f, 1.0f, 0.0f, 1.0f)
            : FLinearColor(1.0f, 0.0f, 1.0f, 1.0f);
        ColorOverLife->SetStartColor(Tint);
        ColorOverLife->SetEndColor(FLinearColor(Tint.R, Tint.G, Tint.B, 0.0f));
        // Exercise the existing rotation modules at 90 degrees per second.
        RotationRate->SetRotationRate(PI * 0.5f);
        // Reuse the existing texture/tint material for initial sprite rendering.
        // The asset manager owns this material; the test settings only reference it.
        UMaterial* Material = UAssetManager::GetAssetByPath<UMaterial>("SpriteMaterial");
        if (!Material)
        {
            HTR_LOG(Warning, "[ParticlePlaybackTest] SpriteMaterial is unavailable; playback has no material.");
        }

        Required->SetMaterial(Material);
        Required->EnableLocalSpace(bLocal);

        LOD->RequiredModule = Required.get();
        LOD->SpawnModule = Spawn.get();
        LOD->Modules.Add(Lifetime.get());
        LOD->Modules.Add(Location.get());
        LOD->Modules.Add(Velocity.get());
        LOD->Modules.Add(Rotation.get());
        LOD->Modules.Add(RotationRate.get());
        LOD->Modules.Add(Size.get());
        LOD->Modules.Add(SizeOverLife.get());
        LOD->Modules.Add(Color.get());
        LOD->Modules.Add(ColorOverLife.get());
        Emitter->LODLevels.Add(LOD.get());
        System->Emitters.Add(Emitter.get());
    }
};

// Temporary comparison: green follows the component; magenta stays in world space.
struct FParticlePlaybackTestSettings
{
    std::unique_ptr<UParticleSystem> System;
    std::unique_ptr<FParticlePlaybackEmitterTestSettings> LocalEmitter;
    std::unique_ptr<FParticlePlaybackEmitterTestSettings> WorldEmitter;

    explicit FParticlePlaybackTestSettings(UObject* Outer)
        : System(NewObject<UParticleSystem>(Outer))
        , LocalEmitter(std::make_unique<FParticlePlaybackEmitterTestSettings>(System.get(), true))
        , WorldEmitter(std::make_unique<FParticlePlaybackEmitterTestSettings>(System.get(), false))
    {
    }
};
