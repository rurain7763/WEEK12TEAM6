-- Mercy 3인칭 조작 (물리 없음, Z = 0 고정)
--   WASD: 카메라 기준 이동 (애님그래프 Speed 파라미터로 Idle <-> Run)
--   마우스 오른쪽 드래그: 카메라 회전
--   마우스 왼쪽 누르고 있기: 힐 (애님그래프 Heal 파라미터, 카메라가 보는 쪽으로 몸을 돌려 지팡이를 겨눔)
--     + 힐 빔 표시. 누르는 순간 HealRange 안의 HealBeacon을 대상으로 잡고, 빔이 그 액터를 따라간다
-- APawn 루트는 PlayerController가 매 프레임 카메라 Yaw로 돌리므로,
-- 캐릭터가 바라보는 방향은 메시 컴포넌트의 상대 Yaw로 따로 준다. (메시는 +X를 보도록 export됨)
local MouseSensitivity = 0.1
local MoveSpeed = 5.5        -- m/s, 오버워치 기본 이동 속도 (Staff_Run 보폭과 대략 맞음)
local TurnSpeed = 720.0      -- deg/s, 이동 방향으로 몸을 돌리는 속도

local Acceleration = 40.0   -- m/s^2, 정지 -> 최고 속도까지 약 0.14초
local Deceleration = 30.0   -- m/s^2, 손을 뗀 뒤 멈출 때까지 약 0.18초

local HealRange = 15.0             -- m, 오버워치 메르시 힐 사거리. 넘어가면 대상을 놓는다
local HealTargetName = "HealBeacon" -- 힐 대상으로 잡을 액터 이름

local mesh
local animInstance
local beam                    -- 힐 빔 파티클 컴포넌트 (Mercy_HealBeam.particlesys)
local healTarget              -- 지금 빔이 붙어 있는 액터. 없으면 빔은 Default(지팡이 정면 직선)
local wasHealing = false
local facingYaw = 0.0
local velX, velY = 0.0, 0.0   -- 월드 속도 (m/s)

function Init()
end

function BeginPlay()
    mesh = owner:FindComponentByClass(SkeletalMeshComponent.StaticClass())
    animInstance = mesh and mesh:GetAnimInstance()
    beam = owner:FindComponentByClass(ParticleSystemComponent.StaticClass())
    if beam then
        beam:SetVisible(false)
    end
    facingYaw = owner:GetControlRotation().Yaw
    GamePlayStatics.SetControlRotation(owner, Rotator.new(17.0, facingYaw, 0.0))
end

-- 현재 각도에서 목표 각도로 최대 maxStep만큼, 짧은 쪽으로 돈다
local function TurnTowards(current, target, maxStep)
    local diff = (target - current + 180.0) % 360.0 - 180.0
    if math.abs(diff) <= maxStep then
        return target
    end
    return current + (diff > 0 and maxStep or -maxStep)
end

local function UpdateCamera()
    if Input.IsMouseDown(MouseButton.Right) then
        local dx, dy = Input.GetMouseDelta()
        owner:AddYawInput(dx * MouseSensitivity)
        owner:AddPitchInput(dy * MouseSensitivity)
    end
end

-- 현재 속도(vx, vy)를 목표 속도(tx, ty)로 deltaTime만큼 다가가게 한 새 속도를 돌려준다.
-- 목표가 0이면 멈추는 중, 아니면 출발/방향 전환 중이다. (Acceleration / Deceleration 사용)
local function UpdateVelocity(vx, vy, tx, ty, deltaTime)
    -- 일정 가속: 목표와의 차이 벡터 방향으로 최대 rate * deltaTime만큼 다가간다 (정해진 시간 안에 정확히 도달)
    local dx, dy = tx - vx, ty - vy
    local dist = math.sqrt(dx * dx + dy * dy)
    local step = ((tx == 0.0 and ty == 0.0) and Deceleration or Acceleration) * deltaTime
    if dist <= step then
        return tx, ty
    end
    return vx + dx / dist * step, vy + dy / dist * step
end

local function UpdateMovement(deltaTime, healing)
    local controlYaw = owner:GetControlRotation().Yaw
    local forward = (Input.IsKeyDown(KeyCode.W) and 1 or 0) - (Input.IsKeyDown(KeyCode.S) and 1 or 0)
    local right = (Input.IsKeyDown(KeyCode.D) and 1 or 0) - (Input.IsKeyDown(KeyCode.A) and 1 or 0)

    -- 카메라 Yaw 기준 입력 방향 (X 전방, Y 오른쪽)의 목표 속도
    local targetX, targetY = 0.0, 0.0
    if forward ~= 0 or right ~= 0 then
        local rad = math.rad(controlYaw + math.deg(math.atan(right, forward)))
        targetX, targetY = math.cos(rad) * MoveSpeed, math.sin(rad) * MoveSpeed
    end
    velX, velY = UpdateVelocity(velX, velY, targetX, targetY, deltaTime)

    local speed = math.sqrt(velX * velX + velY * velY)
    local moveYaw = math.deg(math.atan(velY, velX))
    if speed > 0.0 then
        local loc = owner:GetActorLocation()
        owner:SetActorLocation(Vector.new(loc.x + velX * deltaTime, loc.y + velY * deltaTime, 0.0))
    end

    if speed > 0.1 then   -- 움직이는 중에는 힐 여부와 상관없이 이동 방향을 본다
        facingYaw = TurnTowards(facingYaw, moveYaw, TurnSpeed * deltaTime)
    elseif healing then   -- 서서 힐할 때만 카메라가 겨누는 방향을 본다 (Staff_Heal은 지팡이가 몸 정면을 가리키는 프레임)
        facingYaw = TurnTowards(facingYaw, controlYaw, TurnSpeed * deltaTime)
    end

    if animInstance then
        animInstance:SetFloat("Speed", speed)
        -- 몸이 보는 방향 기준 이동 방향 (-180..180, +가 오른쪽). HealRun 블렌드스페이스가 8방향 달리기를 고른다
        animInstance:SetFloat("MoveAngle", (moveYaw - facingYaw + 180.0) % 360.0 - 180.0)
    end

    -- 루트가 카메라 Yaw를 따라 돌아도 캐릭터는 facingYaw를 유지
    if mesh then
        mesh:SetRelativeRotation(Rotator.new(0.0, facingYaw - controlYaw, 0.0))
    end
end

local function DistanceTo(actor)
    local a, b = owner:GetActorLocation(), actor:GetActorLocation()
    local dx, dy, dz = b.x - a.x, b.y - a.y, b.z - a.z
    return math.sqrt(dx * dx + dy * dy + dz * dz)
end

-- 누르는 순간 사거리 안의 대상을 잡고, 누르는 동안 유지하다가 사거리를 벗어나거나 놓으면 푼다.
-- 빔 끝점은 C++(BeamTarget 모듈)가 매 틱 대상 위치를 읽어 정한다. 여기서는 대상 지정과 표시만 한다
local function UpdateBeam(healing)
    if not beam then
        return
    end

    if healing and not wasHealing then
        local candidate = GamePlayStatics.GetActorByName(owner, HealTargetName)
        if candidate and DistanceTo(candidate) <= HealRange then
            healTarget = candidate
        end
    end
    if not healing or (healTarget and DistanceTo(healTarget) > HealRange) then
        healTarget = nil
    end
    wasHealing = healing

    beam:SetBeamTargetActor(healTarget)   -- nil이면 Default 끝점 (지팡이 정면 × Distance)
    beam:SetVisible(healing)
end

function Tick(deltaTime)
    if not owner:IsInputEnabled() then
        return
    end

    local healing = Input.IsMouseDown(MouseButton.Left)
    if animInstance then
        animInstance:SetBool("Heal", healing)
    end

    UpdateCamera()
    UpdateMovement(deltaTime, healing)
    UpdateBeam(healing)
end

function EndPlay()
    if beam then
        beam:SetBeamTargetActor(nil)   -- 비소유 포인터를 남기지 않는다
    end
    mesh = nil
    animInstance = nil
    beam = nil
    healTarget = nil
    wasHealing = false
    velX, velY = 0.0, 0.0
end
