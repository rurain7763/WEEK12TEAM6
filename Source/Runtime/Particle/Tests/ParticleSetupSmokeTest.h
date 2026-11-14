#pragma once

// Temporary test setup only. Does not implement particle simulation or rendering.
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
#include "Core/EngineLog.h"
#include <cmath>
#include <memory>
#include <type_traits>

namespace ParticleSetupSmokeTest
{
    // Test observer only: records dispatch without changing simulation state.
    class UComponentTickProbe : public UParticleModule
    {
        DECLARE_CLASS(UComponentTickProbe, UParticleModule)
    public:
        UComponentTickProbe() { bUpdateModule = true; }
        void Update(const FUpdateContext& Context) override
        {
            LastInstance = Context.Emitter;
            LastDeltaTime = Context.DeltaTime;
            ++CallCount;
        }
        FParticleEmitterInstance* LastInstance = nullptr;
        float LastDeltaTime = 0.0f;
        int32 CallCount = 0;
    };

    template <typename TInstance = FParticleEmitterInstance>
    inline void Run(UObject* Outer)
    {
        // Keep the test buildable while the trainee's instance type is incomplete.
        if constexpr (!std::is_default_constructible_v<TInstance>)
        {
            HTR_LOG(Warning, "[ParticleSetupTest] SKIP: FParticleEmitterInstance needs a public default constructor.");
        }
        else
        {
            // Declaration order makes the instance die before its settings.
            std::unique_ptr<UParticleEmitter> Emitter(NewObject<UParticleEmitter>(Outer));
            std::unique_ptr<UParticleLODLevel> LOD(NewObject<UParticleLODLevel>(Emitter.get()));
            std::unique_ptr<UParticleModuleLifeTime> Lifetime(NewObject<UParticleModuleLifeTime>(Emitter.get()));
            std::unique_ptr<UParticleModuleVelocity> Velocity(NewObject<UParticleModuleVelocity>(Emitter.get()));
            std::unique_ptr<UParticleModuleLocation> Location(NewObject<UParticleModuleLocation>(Emitter.get()));
            std::unique_ptr<UParticleModuleSpawn> Spawn(NewObject<UParticleModuleSpawn>(Emitter.get()));
            std::unique_ptr<UParticleModuleSize> Size(NewObject<UParticleModuleSize>(Emitter.get()));
            std::unique_ptr<UParticleModuleSizeOfLifeTime> SizeOverLife(NewObject<UParticleModuleSizeOfLifeTime>(Emitter.get()));
            std::unique_ptr<UParticleModuleColor> Color(NewObject<UParticleModuleColor>(Emitter.get()));
            std::unique_ptr<UParticleModuleColorOverLife> ColorOverLife(NewObject<UParticleModuleColorOverLife>(Emitter.get()));
            if (!Emitter || !LOD || !Lifetime || !Velocity || !Location || !Spawn || !Size || !SizeOverLife ||
                !Color || !ColorOverLife)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Could not create test settings.");
                return;
            }

            LOD->Modules.Add(Lifetime.get());
            LOD->Modules.Add(Velocity.get());
            LOD->Modules.Add(Location.get());
            LOD->Modules.Add(Size.get());
            Emitter->LODLevels.Add(LOD.get());

            TInstance Instance;
            Instance.InitParameters(Emitter.get(), nullptr);
            Instance.Initialize();

            if (Instance.CurrentLODLevel != LOD.get() || !Instance.ParticleData ||
                !Instance.ParticleIndices || Instance.ParticleStride < sizeof(FBaseParticle) ||
                Instance.MaxActiveParticles != 100 || Instance.ActiveParticles != 0)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Instance initialization.");
                return;
            }

            // A nonzero starting value ensures the Location module actually writes its default.
            FBaseParticle LocationProbe{};
            LocationProbe.Location = FVector(3.0f, 4.0f, 5.0f);
            UParticleModule::FSpawnContext LocationContext{};
            LocationContext.Emitter = &Instance;
            LocationContext.Particle = &LocationProbe;
            Location->Spawn(LocationContext);
            if (LocationProbe.Location.X != 0.0f || LocationProbe.Location.Y != 0.0f ||
                LocationProbe.Location.Z != 0.0f)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Location module should set the initial position to zero.");
                return;
            }

            // Distinct sentinel values verify both size fields are written by the module.
            FBaseParticle SizeProbe{};
            SizeProbe.BaseSize = FVector(2.0f, 3.0f, 4.0f);
            SizeProbe.Size = FVector(5.0f, 6.0f, 7.0f);
            UParticleModule::FSpawnContext SizeContext{};
            SizeContext.Emitter = &Instance;
            SizeContext.Particle = &SizeProbe;
            Size->Spawn(SizeContext);
            auto HasUnitSize = [](const FBaseParticle& Value)
            {
                return Value.BaseSize.X == 1.0f && Value.BaseSize.Y == 1.0f && Value.BaseSize.Z == 1.0f &&
                    Value.Size.X == 1.0f && Value.Size.Y == 1.0f && Value.Size.Z == 1.0f;
            };
            if (!HasUnitSize(SizeProbe))
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Size module should set both BaseSize and Size to (1, 1, 1).");
                return;
            }

            Instance.SpawnParticle(FVector::ZeroVector);
            if (Instance.ActiveParticles != 1 || Instance.ParticleIndices[0] >= Instance.MaxActiveParticles)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Single particle spawn.");
                return;
            }

            const uint16 Slot = Instance.ParticleIndices[0];
            const FBaseParticle* Particle = reinterpret_cast<const FBaseParticle*>(
                Instance.ParticleData + Slot * Instance.ParticleStride);
            if (!HasUnitSize(*Particle))
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Expected spawned BaseSize and Size (1, 1, 1).");
                return;
            }
            if (Particle->Location.X != 0.0f || Particle->Location.Y != 0.0f || Particle->Location.Z != 0.0f)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Expected spawned position (0, 0, 0).");
                return;
            }
            if (Particle->RelativeTime != 0.0f || !std::isfinite(Particle->InvMaxLifeTime) ||
                std::abs(Particle->InvMaxLifeTime - 0.2f) > 0.00001f)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Expected age 0 and inverse lifetime 0.2, got {} and {}.",
                    Particle->RelativeTime, Particle->InvMaxLifeTime);
                return;
            }

            Instance.KillParticle(0);
            if (Instance.ActiveParticles != 0)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Particle removal.");
                return;
            }
            Instance.SpawnParticle(FVector::ZeroVector);
            if (Instance.ActiveParticles != 1 || Instance.ParticleIndices[0] != Slot)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Removed slot reuse.");
                return;
            }

            // The respawned particle starts at age zero with the default upward velocity.
            Particle = reinterpret_cast<const FBaseParticle*>(
                Instance.ParticleData + Slot * Instance.ParticleStride);
            if (!std::isfinite(Particle->Velocity.X) || !std::isfinite(Particle->Velocity.Y) ||
                !std::isfinite(Particle->Velocity.Z) || Particle->Velocity.X != 0.0f ||
                Particle->Velocity.Y != 0.0f || Particle->Velocity.Z != 1.0f)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Expected initial velocity (0, 0, 1).");
                return;
            }
            const FVector InitialLocation = Particle->Location;
            Instance.Tick(0.5f);
            if (Instance.ActiveParticles != 1)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Particle should survive the first 0.5s.");
                return;
            }
            Particle = reinterpret_cast<const FBaseParticle*>(
                Instance.ParticleData + Instance.ParticleIndices[0] * Instance.ParticleStride);
            const FVector Displacement = Particle->Location - InitialLocation;
            if (!std::isfinite(Particle->RelativeTime) ||
                std::abs(Particle->RelativeTime - 0.1f) > 0.00001f ||
                !std::isfinite(Displacement.X) || !std::isfinite(Displacement.Y) ||
                !std::isfinite(Displacement.Z) || std::abs(Displacement.X) > 0.00001f ||
                std::abs(Displacement.Y) > 0.00001f || std::abs(Displacement.Z - 0.5f) > 0.00001f)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: After 0.5s expected relative age 0.1 and Z displacement 0.5.");
                return;
            }

            // Two simultaneously expired particles exercise removal during iteration.
            Instance.SpawnParticle(FVector::ZeroVector);
            if (Instance.ActiveParticles != 2)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Second particle spawn.");
                return;
            }
            Instance.Tick(5.0f);
            if (Instance.ActiveParticles != 0)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Both particles should have expired.");
                return;
            }

            HTR_LOG(Info, "[ParticleSetupTest] PASS: Lifetime, initial location, initial velocity, initial size, slot reuse, movement, aging and expiration.");

            // Isolate the update-module test from the initial-size checks above.
            LOD->Modules.Add(SizeOverLife.get());
            TInstance SizeInstance;
            SizeInstance.InitParameters(Emitter.get(), nullptr);
            SizeInstance.Initialize();
            SizeInstance.SpawnParticle(FVector::ZeroVector);
            if (SizeInstance.ActiveParticles != 1)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Size-over-life test particle spawn.");
                return;
            }
            FBaseParticle* SizeParticle = SizeInstance.GetParticleFromPosition(0);
            // Nonuniform size checks that every axis uses its own immutable base value.
            SizeParticle->BaseSize = FVector(2.0f, 3.0f, 4.0f);
            SizeParticle->Size = SizeParticle->BaseSize;
            auto CheckLifeSize = [&](float ExpectedAge, float ExpectedScale)
            {
                if (SizeInstance.ActiveParticles != 1) return false;
                const FBaseParticle* Value = SizeInstance.GetParticleFromPosition(0);
                return Value->BaseSize.X == 2.0f && Value->BaseSize.Y == 3.0f && Value->BaseSize.Z == 4.0f &&
                    std::isfinite(Value->RelativeTime) && std::isfinite(Value->Size.X) &&
                    std::isfinite(Value->Size.Y) && std::isfinite(Value->Size.Z) &&
                    std::abs(Value->RelativeTime - ExpectedAge) < 0.00001f &&
                    std::abs(Value->Size.X - 2.0f * ExpectedScale) < 0.00001f &&
                    std::abs(Value->Size.Y - 3.0f * ExpectedScale) < 0.00001f &&
                    std::abs(Value->Size.Z - 4.0f * ExpectedScale) < 0.00001f;
            };
            SizeInstance.Tick(1.25f);
            if (!CheckLifeSize(0.25f, 1.25f))
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: At quarter life expected size (2.5, 3.75, 5), unchanged BaseSize.");
                return;
            }
            SizeInstance.Tick(1.25f);
            if (!CheckLifeSize(0.5f, 1.5f))
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: At half life expected size (3, 4.5, 6), unchanged BaseSize.");
                return;
            }
            SizeInstance.Tick(0.0f);
            if (!CheckLifeSize(0.5f, 1.5f))
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Size should not compound when age stays the same.");
                return;
            }
            HTR_LOG(Info, "[ParticleSetupTest] PASS: Size-over-life update, immutable BaseSize and no compounding.");

            // Check default initialization and overwrite semantics before configuring distinct endpoints.
            auto IsWhite = [](const FLinearColor& Value)
            {
                return Value.R == 1.0f && Value.G == 1.0f && Value.B == 1.0f && Value.A == 1.0f;
            };
            FBaseParticle ColorProbe{};
            ColorProbe.BaseColor = FLinearColor(0.2f, 0.3f, 0.4f, 0.5f);
            ColorProbe.Color = FLinearColor(0.6f, 0.7f, 0.8f, 0.9f);
            UParticleModule::FSpawnContext ColorContext{};
            ColorContext.Emitter = &SizeInstance;
            ColorContext.Particle = &ColorProbe;
            Color->Spawn(ColorContext);
            if (!IsWhite(ColorProbe.BaseColor) || !IsWhite(ColorProbe.Color))
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Initial color module should set BaseColor and Color to white.");
                return;
            }

            LOD->Modules.Add(Color.get());
            LOD->Modules.Add(ColorOverLife.get());
            TInstance ColorInstance;
            ColorInstance.InitParameters(Emitter.get(), nullptr);
            ColorInstance.Initialize();
            ColorInstance.SpawnParticle(FVector::ZeroVector);
            if (ColorInstance.ActiveParticles != 1 ||
                !IsWhite(ColorInstance.GetParticleFromPosition(0)->Color) ||
                !IsWhite(ColorInstance.GetParticleFromPosition(0)->BaseColor))
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Spawned color initialization.");
                return;
            }
            FBaseParticle* ColorParticle = ColorInstance.GetParticleFromPosition(0);
            ColorParticle->BaseColor = FLinearColor(0.2f, 0.3f, 0.4f, 0.5f);
            ColorParticle->Color = FLinearColor(0.6f, 0.7f, 0.8f, 0.9f);
            ColorInstance.Tick(2.5f);
            if (ColorInstance.ActiveParticles != 1)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Color test particle should survive to half life.");
                return;
            }
            ColorParticle = ColorInstance.GetParticleFromPosition(0);
            if (!IsWhite(ColorParticle->Color) || ColorParticle->BaseColor.R != 0.2f ||
                ColorParticle->BaseColor.G != 0.3f || ColorParticle->BaseColor.B != 0.4f ||
                ColorParticle->BaseColor.A != 0.5f)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Color-over-life should overwrite Color while preserving BaseColor.");
                return;
            }
            ColorOverLife->SetStartColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
            ColorOverLife->SetEndColor(FLinearColor(0.0f, 0.0f, 1.0f, 0.0f));
            auto CheckInterpolatedColor = [&](float R, float B, float A)
            {
                if (ColorInstance.ActiveParticles != 1) return false;
                const FBaseParticle* Value = ColorInstance.GetParticleFromPosition(0);
                return std::isfinite(Value->Color.R) && std::isfinite(Value->Color.G) &&
                    std::isfinite(Value->Color.B) && std::isfinite(Value->Color.A) &&
                    std::abs(Value->Color.R - R) < 0.00001f &&
                    std::abs(Value->Color.G) < 0.00001f &&
                    std::abs(Value->Color.B - B) < 0.00001f &&
                    std::abs(Value->Color.A - A) < 0.00001f &&
                    Value->BaseColor.R == 0.2f && Value->BaseColor.G == 0.3f &&
                    Value->BaseColor.B == 0.4f && Value->BaseColor.A == 0.5f;
            };
            // The particle is already at half life; a zero step must apply the new settings.
            ColorInstance.Tick(0.0f);
            if (!CheckInterpolatedColor(0.5f, 0.5f, 0.5f))
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: At half life expected RGBA (0.5, 0, 0.5, 0.5), unchanged BaseColor.");
                return;
            }
            ColorInstance.Tick(1.25f);
            if (!CheckInterpolatedColor(0.25f, 0.75f, 0.25f))
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: At three-quarter life expected RGBA (0.25, 0, 0.75, 0.25), unchanged BaseColor.");
                return;
            }
            ColorInstance.Tick(0.0f);
            if (!CheckInterpolatedColor(0.25f, 0.75f, 0.25f))
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Color should not compound when age stays the same.");
                return;
            }
            HTR_LOG(Info, "[ParticleSetupTest] PASS: Initial color, red-to-blue interpolation, alpha fade and immutable BaseColor.");

            // A separate instance keeps automatic spawning out of the manual tests above.
            LOD->SpawnModule = Spawn.get();
            TInstance AutoInstance;
            AutoInstance.InitParameters(Emitter.get(), nullptr);
            AutoInstance.Initialize();
            if (!AutoInstance.ParticleData || !AutoInstance.ParticleIndices ||
                AutoInstance.MaxActiveParticles != 100 || AutoInstance.ActiveParticles != 0 ||
                AutoInstance.SpawnFraction != 0.0f)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Automatic spawn initialization.");
                return;
            }

            auto CheckAutoSpawn = [&](int32 ExpectedCount, float ExpectedFraction, const char* Stage)
            {
                if (AutoInstance.ActiveParticles != ExpectedCount ||
                    !std::isfinite(AutoInstance.SpawnFraction) ||
                    std::abs(AutoInstance.SpawnFraction - ExpectedFraction) > 0.00001f)
                {
                    HTR_LOG(Error, "[ParticleSetupTest] FAIL: {}: expected count {} / fraction {}, got {} / {}.",
                        Stage, ExpectedCount, ExpectedFraction,
                        AutoInstance.ActiveParticles, AutoInstance.SpawnFraction);
                    return false;
                }
                return true;
            };

            // Binary-exact time steps avoid ambiguous rounding at integer spawn thresholds.
            // Default rate: 10/s. Emitter-configured capacity: 100.
            // No particle expires during this sequence.
            AutoInstance.Tick(0.125f);
            if (!CheckAutoSpawn(1, 0.25f, "First accumulation")) return;
            AutoInstance.Tick(0.125f);
            if (!CheckAutoSpawn(2, 0.5f, "Fraction carried to next frame")) return;
            AutoInstance.Tick(0.25f);
            if (!CheckAutoSpawn(5, 0.0f, "Whole accumulated requests")) return;

            // Fill remaining slots explicitly instead of advancing past particle lifetimes.
            const int32 Capacity = AutoInstance.MaxActiveParticles;
            const int32 RemainingSlots = Capacity - AutoInstance.ActiveParticles;
            for (int32 Index = 0; Index < RemainingSlots; ++Index)
            {
                AutoInstance.SpawnParticle(FVector::ZeroVector);
            }
            if (!CheckAutoSpawn(Capacity, 0.0f, "Fill configured capacity")) return;
            AutoInstance.Tick(0.125f);
            if (!CheckAutoSpawn(Capacity, 0.25f, "Drop excess integer requests at full capacity")) return;

            AutoInstance.KillParticle(0);
            AutoInstance.Tick(0.0f);
            if (!CheckAutoSpawn(Capacity - 1, 0.25f, "No deferred integer spawn debt")) return;

            HTR_LOG(Info, "[ParticleSetupTest] PASS: Automatic spawn accumulation, fraction carry and capacity handling.");

            // Exercise the component API directly; this is not a world tick-registration test.
            std::unique_ptr<UParticleSystem> System(NewObject<UParticleSystem>(Outer));
            std::unique_ptr<UComponentTickProbe> Probe(NewObject<UComponentTickProbe>(Emitter.get()));
            if (!System || !Probe)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Component test settings creation.");
                return;
            }
            System->Emitters.Add(Emitter.get());
            LOD->Modules.Add(Probe.get());
            // Declared last so component instances are destroyed before their settings.
            std::unique_ptr<UParticleSystemComponent> Component(NewObject<UParticleSystemComponent>(Outer));
            if (!Component || !Component->PrimaryComponentTick.bCanEverTick)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Component creation or tick capability.");
                return;
            }
            Component->SetTemplate(System.get());
            Component->BeginPlay();
            Component->TickComponent(0.125f);
            if (Probe->CallCount != 1 || Probe->LastDeltaTime != 0.125f || !Probe->LastInstance ||
                Probe->LastInstance->EmitterTemplate != Emitter.get() ||
                Probe->LastInstance->ActiveParticles != 1)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Component BeginPlay/CreateInstance/Tick dispatch.");
                return;
            }
            Component->TickComponent(0.125f);
            if (Probe->CallCount != 2 || Probe->LastInstance->ActiveParticles != 2 ||
                !std::isfinite(Probe->LastInstance->SpawnFraction) ||
                std::abs(Probe->LastInstance->SpawnFraction - 0.5f) > 0.00001f)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Component should retain and advance its emitter instance.");
                return;
            }
            Component->EndPlay();
            // Do not dereference the observer's old instance pointer after EndPlay.
            Probe->LastInstance = nullptr;
            Component->TickComponent(0.0f);
            if (Probe->CallCount != 2)
            {
                HTR_LOG(Error, "[ParticleSetupTest] FAIL: Component should not tick instances after EndPlay.");
                return;
            }
            Component->ClearEmitterInstances();
            Component.reset();
            HTR_LOG(Info, "[ParticleSetupTest] PASS: Component initialization, tick forwarding and cleanup API.");
        }
    }
}
