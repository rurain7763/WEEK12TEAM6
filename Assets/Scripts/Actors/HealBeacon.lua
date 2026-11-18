-- 힐 빔 테스트용 비콘: 처음 위치를 중심으로 수평 원을 그리며 돈다 (빔 끝이 움직이는 대상을 따라가는지 확인용)
local Radius = 3.0        -- m
local AngularSpeed = 0.6  -- rad/s, 한 바퀴 약 10초

local center
local angle = 0.0

function BeginPlay()
    center = owner:GetActorLocation()
end

function Tick(deltaTime)
    if not center then
        return
    end
    angle = angle + AngularSpeed * deltaTime
    owner:SetActorLocation(Vector.new(center.x + math.cos(angle) * Radius, center.y + math.sin(angle) * Radius, center.z))
end

function EndPlay()
    center = nil
end
