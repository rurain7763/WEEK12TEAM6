#include "EnginePCH.h"
#include "LuaScriptInstance.h"
#include "Core/PathUtils.h"
#include <fstream>
#include <iterator>

namespace
{
    bool ReadScriptSource(const FString& Path, FString& Source)
    {
        std::ifstream File(PathUtils::FromUtf8(Path), std::ios::binary);
        if (!File) return false;
        Source.assign(std::istreambuf_iterator<char>(File), std::istreambuf_iterator<char>());
        if (File.bad()) return false;
        // Match Lua's file loader handling of a UTF-8 BOM and a leading shebang.
        if (Source.compare(0, 3, "\xEF\xBB\xBF") == 0) Source.erase(0, 3);
        if (!Source.empty() && Source[0] == '#')
        {
            const auto End = Source.find('\n');
            Source.replace(0, End == FString::npos ? Source.size() : End, "");
        }
        return true;
    }
}

#include "GameFramework/Actor/PlayerActor.h"
#include "GameFramework/Actor/EnemyActor.h"
#include "GameFramework/Pawn.h"

void FLuaScriptInstance::Load(sol::state& LuaState, AActor* InOwner, const FString& InScriptPath)
{
    bLoaded = false;
    bHasBegunPlay = false;

    InitFunction = {};
    BeginPlayFunction = {};
    TickFunction = {};
    EndPlayFunction = {};

    Owner = InOwner;
    ScriptPath = InScriptPath;
    State = LuaState.create_table();

    if (!Owner || ScriptPath.empty())
    {
        return;
    }

    // 새로운 environment를 만들고 없는 값을 공용 전역에서 찾음
    Environment = sol::environment(LuaState, sol::create, LuaState.globals());

    if (APlayerActor* Player = Cast<APlayerActor>(Owner))
    {
        Environment["owner"] = Player;
    }
    else if (AEnemyActor* Enemy = Cast<AEnemyActor>(Owner))
    {
        Environment["owner"] = Enemy;
    }
    else if (APawn* Pawn = Cast<APawn>(Owner))
    {
        Environment["owner"] = Pawn;
    }
    else
    {
        Environment["owner"] = Owner;
    }

    Environment["state"] = State;

    FString Source;
    if (!ReadScriptSource(ScriptPath, Source))
    {
        HTR_LOG(Error, "[Lua] Cannot read script: {}", ScriptPath);
        return;
    }
    sol::protected_function_result Result = LuaState.safe_script(Source, Environment, sol::script_default_on_error, "@" + ScriptPath);
    if (!Result.valid())
    {
        sol::error e = Result;
        HTR_LOG(Error, "[Lua] Load failed: {}, Error: {}", ScriptPath, e.what());
        return;
    }

    InitFunction = Environment["Init"];
    BeginPlayFunction = Environment["BeginPlay"];
    TickFunction = Environment["Tick"];
    EndPlayFunction = Environment["EndPlay"];
    OnComponentBeginOverlapFunc = Environment["OnComponentBeginOverlap"];
    OnComponentEndOverlapFunc = Environment["OnComponentEndOverlap"];
    OnComponentHitFunc = Environment["OnComponentHit"];

    bLoaded = true;
    bHasBegunPlay = false;
    LastWriteTime = fs::last_write_time(PathUtils::FromUtf8(ScriptPath));
}

void FLuaScriptInstance::Reload(sol::state& LuaState)
{
    if (!State.valid())
    {
        State = LuaState.create_table();
    }

    sol::environment NewEnvironment(LuaState, sol::create, LuaState.globals());
    if (APlayerActor* Player = Cast<APlayerActor>(Owner))
    {
        NewEnvironment["owner"] = Player;
    }
    else if (AEnemyActor* Enemy = Cast<AEnemyActor>(Owner))
    {
        NewEnvironment["owner"] = Enemy;
    }
    else if (APawn* Pawn = Cast<APawn>(Owner))
    {
        NewEnvironment["owner"] = Pawn;
    }
    else
    {
        NewEnvironment["owner"] = Owner;
    }
    NewEnvironment["state"] = State;

    FString Source;
    if (!ReadScriptSource(ScriptPath, Source))
    {
        HTR_LOG(Error, "[Lua] Cannot read script: {}", ScriptPath);
        return;
    }
    sol::protected_function_result Result = LuaState.safe_script(Source, NewEnvironment, sol::script_default_on_error, "@" + ScriptPath);

    if (!Result.valid())
    {
        sol::error Error = Result;
        HTR_LOG(Error, "[Lua] Reload failed: {}, Error: {}", ScriptPath, Error.what());
        return;
    }

    Environment = std::move(NewEnvironment);
    InitFunction = Environment["Init"];
    BeginPlayFunction = Environment["BeginPlay"];
    TickFunction = Environment["Tick"];
    EndPlayFunction = Environment["EndPlay"];
    OnComponentBeginOverlapFunc = Environment["OnComponentBeginOverlap"];
    OnComponentEndOverlapFunc = Environment["OnComponentEndOverlap"];
    OnComponentHitFunc = Environment["OnComponentHit"];

    Init();
}

void FLuaScriptInstance::Init()
{
    if (!bLoaded)
    {
        return;
    }

    sol::protected_function_result Result = InitFunction();
    if (!Result.valid())
    {
        sol::error e = Result;
        HTR_LOG(Error, "[Lua] Init failed: {}, Error: {}", ScriptPath, e.what());
    }
}

void FLuaScriptInstance::BeginPlay()
{
    if (!bLoaded || bHasBegunPlay)
    {
        return;
    }
    bHasBegunPlay = true;

    sol::protected_function_result Result = BeginPlayFunction();
    if (!Result.valid())
    {
        sol::error e = Result;
        HTR_LOG(Error, "[Lua] BeginPlay failed: {}, Error: {}", ScriptPath, e.what());
    }
}

void FLuaScriptInstance::Tick(sol::state& LuaState, float DeltaTime)
{
    if (!bLoaded || !bHasBegunPlay)
    {
        return;
    }

    sol::protected_function_result Result = TickFunction(DeltaTime);
    if (!Result.valid())
    {
        sol::error e = Result;
        HTR_LOG(Error, "[Lua] Tick failed: {}, Error: {}", ScriptPath, e.what());
    }

    HotReloadCheckTimer += DeltaTime;

    if (HotReloadCheckTimer >= 0.5f)
    {
        HotReloadCheckTimer = 0.0f;

        std::error_code Error;
        const fs::file_time_type CurrentWriteTime = fs::last_write_time(PathUtils::FromUtf8(ScriptPath), Error);

        if (Error)
        {
            return;
        }

        if (CurrentWriteTime != LastWriteTime)
        {
            LastWriteTime = CurrentWriteTime;
            Reload(LuaState);
        }
    }
}

void FLuaScriptInstance::EndPlay()
{
    if (!bHasBegunPlay)
    {
        return;
    }
    bHasBegunPlay = false;

    sol::protected_function_result Result = EndPlayFunction();
    if (!Result.valid())
    {
        sol::error e = Result;
        HTR_LOG(Error, "[Lua] EndPlay failed: {}, Error: {}", ScriptPath, e.what());
    }
}

void FLuaScriptInstance::OnComponentBeginOverlap(UPrimitiveComponent* Other)
{
    if (!OnComponentBeginOverlapFunc)
    {
        return;
    }

    sol::protected_function_result Result = OnComponentBeginOverlapFunc(Other);
    if (!Result.valid())
    {
        sol::error e = Result;
        HTR_LOG(Error, "[Lua] EndPlay failed: {}, Error: {}", ScriptPath, e.what());
    }
}

void FLuaScriptInstance::OnComponentEndOverlap(UPrimitiveComponent* Other)
{
    if (!OnComponentEndOverlapFunc)
    {
        return;
    }

    sol::protected_function_result Result = OnComponentEndOverlapFunc(Other);
    if (!Result.valid())
    {
        sol::error e = Result;
        HTR_LOG(Error, "[Lua] EndPlay failed: {}, Error: {}", ScriptPath, e.what());
    }
}

void FLuaScriptInstance::OnComponentHit(UPrimitiveComponent* Other, FHitResult& HitResult, bool bSelfMoved)
{
    if (!OnComponentHitFunc)
    {
        return;
    }

    sol::protected_function_result Result = OnComponentHitFunc(Other, HitResult, bSelfMoved);
    if (!Result.valid())
    {
        sol::error e = Result;
        HTR_LOG(Error, "[Lua] EndPlay failed: {}, Error: {}", ScriptPath, e.what());
    }
}
