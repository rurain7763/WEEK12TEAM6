#pragma once

#include "LightweightStats.h"

DECLARE_CYCLE_STAT("Particle Tick", STAT_ParticleTick);
DECLARE_CYCLE_STAT("Particle Build RenderData", STAT_ParticleBuildRenderData);

struct FParticleFrameStats
{
	static constexpr int32 MaxLODBuckets = 4;	// 마지막 칸은 그 이상 LOD를 합산

	// 개수 (이미터 종류별 활성 입자)
	int32 Systems = 0;
	int32 Emitters = 0;
	int32 SpriteParticles = 0;
	int32 MeshParticles = 0;
	int32 StripParticles = 0;	// Beam, Ribbon

	// 이번 프레임 생성·소멸
	int32 Spawned = 0;
	int32 Killed = 0;

	// 메모리 (ParticleData + 인덱스 배열)
	int64 ParticleDataBytes = 0;
	int64 ParticleIndexBytes = 0;

	// 렌더 (모든 View 합산)
	int32 RenderPackets = 0;		// 프록시가 제출한 패킷
	int32 DrawCalls = 0;			// 반투명은 같은 머티리얼·지오메트리끼리 인스턴싱으로 묶임
	int32 InstancedParticles = 0;	// 인스턴싱 드로우로 그린 패킷 수
	int64 DynamicUploadBytes = 0;

	// LOD 단계별 이미터 수
	int32 LODEmitters[MaxLODBuckets] = {};

	// 충돌
	int32 CollisionTraces = 0;
	int32 CollisionHits = 0;
	int32 CollisionEvents = 0;
};

// 프레임 동안 Pending에 모으고 EndFrame에서 표시용 Last로 넘긴다
class FParticleStats
{
public:
	static FParticleFrameStats& GetPending() { return Pending; }
	static const FParticleFrameStats& Get() { return Last; }

	static void EndFrame()
	{
		Last = Pending;
		Pending = FParticleFrameStats{};
	}

private:
	inline static FParticleFrameStats Pending;
	inline static FParticleFrameStats Last;
};
