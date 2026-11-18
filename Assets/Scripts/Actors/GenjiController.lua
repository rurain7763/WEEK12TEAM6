-- 겐지 용검 시연용 (조작 없음): SwingInterval마다 Swing 트리거를 걸어 2연속 베기를 한 번 한다
--   Genji_Blade.animgraph: Idle --(Swing 트리거)--> Swing1 --(끝나면)--> Swing2 --(끝나면)--> Idle
--   칼끝 궤적 리본(BladeTrail, Genji_BladeTrail.particlesys)은 베는 동안만 보인다. 리본 점은 C++ LocationBoneSocket
--   모듈이 BladeTip 본 위치에 만들고, 여기서는 표시만 켜고 끈다
local SwingInterval = 2.5   -- s, 한 번의 2연속 베기(약 2.1초) 뒤 잠깐 대기 자세를 보여주는 간격
local SwingDuration = 2.4   -- s, 베기 2번(Swing1·Swing2를 각각 90%까지 재생, 약 2.1초) + 마지막 궤적이 사라지는 시간(번개 수명 0.3초)

local animInstance
local trail
local elapsed = 0.0
local sinceSwing = math.huge   -- 마지막 Swing 트리거 이후 시간. 첫 베기 전에는 리본을 숨긴다

function BeginPlay()
    local mesh = owner:FindComponentByClass(SkeletalMeshComponent.StaticClass())
    animInstance = mesh and mesh:GetAnimInstance()
    trail = owner:FindComponentByClass(ParticleSystemComponent.StaticClass())
    if trail then
        trail:SetVisible(false)
    end
    elapsed = 0.0
    sinceSwing = math.huge
end

function Tick(deltaTime)
    if not animInstance then
        return
    end
    elapsed = elapsed + deltaTime
    sinceSwing = sinceSwing + deltaTime
    if elapsed >= SwingInterval then
        elapsed = elapsed - SwingInterval
        sinceSwing = 0.0
        animInstance:SetTrigger("Swing")
    end
    if trail then
        trail:SetVisible(sinceSwing < SwingDuration)
    end
end

function EndPlay()
    animInstance = nil
    trail = nil
end
