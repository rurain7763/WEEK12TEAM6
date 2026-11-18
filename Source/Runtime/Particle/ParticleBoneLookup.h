#pragma once

class UParticleSystemComponent;

// 파티클 컴포넌트의 주인 액터 -> 스켈레탈 메시 -> 본 이름으로 노드를 찾아 본과 부모 본의 월드 위치를 구한다
// 방향은 부모 -> 본으로 잡는다. 본 축 규약이 FBX 변환을 거치며 바뀌어도 영향을 받지 않는다 (구현: ParticleBeamModules.cpp)
bool FindBoneWorldPoints(const UParticleSystemComponent* PSC, const FString& BoneName, FVector& OutBone, FVector& OutParent);
