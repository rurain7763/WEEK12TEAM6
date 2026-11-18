-- Exhibition playback only: moves an emitter without changing particle modules.
local origin
local elapsed = 0
function Init()
end
function BeginPlay()
    origin = owner:GetActorLocation()
    elapsed = 0
end
function Tick(deltaTime)
    if not origin then return end
    elapsed = elapsed + deltaTime
    owner:SetActorLocation(Vector.new(origin.x + math.sin(elapsed * 1.1) * 3.0,
        origin.y + math.sin(elapsed * 2.2) * 1.2,
        origin.z + math.cos(elapsed * 1.1) * 0.7))
end
function EndPlay()
    origin = nil
end
