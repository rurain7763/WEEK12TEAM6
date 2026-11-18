#pragma once

#include "LuaScriptSystem.h"
#include "GameFramework/Actor.h"

struct FParticleEventCollideData;

class FLuaScriptInstance
{
public:
    void Load(sol::state& LuaState, AActor* InOwner, const FString& InScriptPath);
    void Reload(sol::state& LuaState);

    void Init();
    void BeginPlay();
    void Tick(sol::state& LuaState, float DeltaTime);
    void EndPlay();
    void OnComponentBeginOverlap(UPrimitiveComponent* Other);
    void OnComponentEndOverlap(UPrimitiveComponent* Other);
    void OnComponentHit(UPrimitiveComponent* Other, FHitResult& HitResult, bool bSelfMoved);
    void OnParticleCollide(const FParticleEventCollideData& Event);

    inline sol::table GetTable()
    {
        return State;
    }

private:
    AActor* Owner = nullptr;
    FString ScriptPath;

    sol::environment Environment;
    sol::table State;

    sol::protected_function InitFunction;
    sol::protected_function BeginPlayFunction;
    sol::protected_function TickFunction;
    sol::protected_function EndPlayFunction;
    sol::protected_function OnComponentBeginOverlapFunc;
    sol::protected_function OnComponentEndOverlapFunc;
    sol::protected_function OnComponentHitFunc;
    sol::protected_function OnParticleCollideFunc;

    bool bLoaded = false;
    bool bHasBegunPlay = false;

    fs::file_time_type LastWriteTime;
    float HotReloadCheckTimer = 0.0f;
};