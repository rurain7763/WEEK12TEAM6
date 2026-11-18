-- Exhibition observer only; existing collision/event implementations produce events.
function Init()
end
function BeginPlay()
end
function Tick(deltaTime)
end
function OnParticleCollide(event)
    Log(string.format("[Particle Gallery] %s", event.EventName))
end
function EndPlay()
end
