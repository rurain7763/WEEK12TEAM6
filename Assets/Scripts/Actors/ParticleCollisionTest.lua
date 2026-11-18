-- 파티클 충돌 이벤트 확인용 스크립트
-- PSC의 OnParticleCollide가 Broadcast되면 event(ParticleCollideEvent)와 함께 호출

-- 스크립트 시스템이 nil 검사 없이 호출하므로 빈 함수라도 필요
function Init()
end

function BeginPlay()
end

function Tick(deltaTime)
end

function EndPlay()
end

function OnParticleCollide(event)
    local loc = event.Location
    Log(string.format("[ParticleCollide] %s emitter=%d loc=(%.2f, %.2f, %.2f)",
        event.EventName, event.EmitterIndex, loc.x, loc.y, loc.z))

    -- 이벤트 이름별 소리
    if event.EventName == "FirstHit" then
        GamePlayStatics.PlaySound("Assets/Sounds/Pop.wav", 0.3)
    elseif event.EventName == "KillHit" then
        GamePlayStatics.PlaySound("Assets/Sounds/Hit.wav", 0.2)
    end
end
