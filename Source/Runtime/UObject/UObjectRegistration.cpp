#include "EnginePCH.h"
#include "UObjectRegistration.h"

#include "Animation/AnimationAsset.h"
#include "Animation/AnimDataModel.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNotify.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/Graph/AnimGraph.h"
#include "Animation/Skeleton.h"
#include "Asset/Asset.h"
#include "Asset/AssetManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraModifier_CameraShake.h"
#include "Camera/CameraModifier_FOVTest.h"
#include "Camera/CameraModifier.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/CameraShakeHit.h"
#include "Camera/CameraShakePattern.h"
#include "Camera/CameraShakePatternWave.h"
#include "Camera/CameraShakeSlam.h"
#include "Camera/CameraShakeSourceComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Component/ActorComponent.h"
#include "Component/AmbientLightComponent.h"
#include "Component/BillboardComponent.h"
#include "Component/BoxComponent.h"
#include "Component/CapsuleComponent.h"
#include "Component/CharacterMovementComponent.h"
#include "Component/DecalComponent.h"
#include "Component/DirectionalLightComponent.h"
#include "Component/ExponentialHeightFogComponent.h"
#include "Component/FXSystemComponent.h"
#include "Component/LightComponent.h"
#include "Component/LightComponentBase.h"
#include "Component/MeshComponent.h"
#include "Component/MovementComponent.h"
#include "Component/ParticleSubUVComponent.h"
#include "Component/ParticleSystemComponent.h"
#include "Component/PointLightComponent.h"
#include "Component/PrimitiveComponent.h"
#include "Component/ProjectileMovementComponent.h"
#include "Component/RotatingMovementComponent.h"
#include "Component/SceneComponent.h"
#include "Component/ScriptComponent.h"
#include "Component/ShapeComponent.h"
#include "Component/SkeletalMeshComponent.h"
#include "Component/SkinnedMeshComponent.h"
#include "Component/SphereComponent.h"
#include "Component/SpotLightComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Component/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Game/Bullet.h"
#include "Game/Coin.h"
#include "Game/CoinLauncher.h"
#include "Game/CoinMovementComponent.h"
#include "Game/CoinRain.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Actor/AmbientLightActor.h"
#include "GameFramework/Actor/BillboardActor.h"
#include "GameFramework/Actor/DecalActor.h"
#include "GameFramework/Actor/DirectionalLightActor.h"
#include "GameFramework/Actor/EnemyActor.h"
#include "GameFramework/Actor/ExponentialHeightFogActor.h"
#include "GameFramework/Actor/FakeSpotLightActor.h"
#include "GameFramework/Actor/LuaTestActor.h"
#include "GameFramework/Actor/ParticleActor.h"
#include "GameFramework/Actor/ParticleEventManager.h"
#include "GameFramework/Actor/PlayerActor.h"
#include "GameFramework/Actor/PointLightActor.h"
#include "GameFramework/Actor/RootMotionTestActor.h"
#include "GameFramework/Actor/SkeletalMeshActor.h"
#include "GameFramework/Actor/SpotLightActor.h"
#include "GameFramework/Actor/StaticMeshActor.h"
#include "GameFramework/Actor/TextRenderActor.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpringArmComponent.h"
#include "Particle/Modules/ParticleModuleColor.h"
#include "Particle/Modules/ParticleModuleAccelerationBase.h"
#include "Particle/Modules/ParticleModuleAccelerationConstant.h"
#include "Particle/Modules/ParticleModuleCollisionBase.h"
#include "Particle/Modules/ParticleModuleCollision.h"
#include "Particle/Modules/ParticleModuleEventBase.h"
#include "Particle/Modules/ParticleModuleEventGenerator.h"
#include "Particle/Modules/ParticleModuleColorOverLife.h"
#include "Particle/Modules/ParticleModuleLifeTime.h"
#include "Particle/Modules/ParticleModuleLocation.h"
#include "Particle/Modules/ParticleModuleLocationPrimitiveBox.h"
#include "Particle/Modules/ParticleModuleLocationPrimitiveSphere.h"
#include "Particle/Modules/ParticleModuleLocationPrimitiveCylinder.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Particle/Modules/ParticleModuleRotation.h"
#include "Particle/Modules/ParticleModuleRotationRate.h"
#include "Particle/Modules/ParticleModuleSize.h"
#include "Particle/Modules/ParticleModuleSizeScaleBySpeed.h"
#include "Particle/Modules/ParticleModuleSizeOfLifeTime.h"
#include "Particle/Modules/ParticleModuleSpawn.h"
#include "Particle/Modules/ParticleModuleTypeDataBase.h"
#include "Particle/Modules/ParticleModuleTypeDataBeam.h"
#include "Particle/Modules/ParticleModuleBeamSource.h"
#include "Particle/Modules/ParticleModuleBeamTarget.h"
#include "Particle/Modules/ParticleModuleTypeDataRibbon.h"
#include "Particle/Modules/ParticleModuleTypeDataMesh.h"
#include "Particle/Modules/ParticleModuleVelocity.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleModule.h"
#include "Particle/ParticleSystem.h"
#include "Render/Material.h"
#include "Render/MaterialInterface.h"
#include "Render/Mesh.h"
#include "Render/SkeletalMesh.h"
#include "Render/Texture2D.h"
#include "Sequence/LevelSceneSequencePlayer.h"
#include "Sequence/LevelSequence.h"
#include "Sequence/LevelSequenceActor.h"
#include "Sequence/MovieScene.h"
#include "Sequence/MovieScene3DTransformSection.h"
#include "Sequence/MovieScene3DTransformTrack.h"
#include "Sequence/MovieSceneCameraCutSection.h"
#include "Sequence/MovieSceneCameraCutTrack.h"
#include "Sequence/MovieSceneFadeSection.h"
#include "Sequence/MovieSceneFadeTrack.h"
#include "Sequence/MovieScenePropertyTrack.h"
#include "Sequence/MovieSceneSection.h"
#include "Sequence/MovieSceneSequence.h"
#include "Sequence/MovieSceneSequencePlayer.h"
#include "Text/Font.h"

void RegisterUClasses()
{
	// Manual runtime registration list. Keep this entry point when automating it later.
	// Referencing each class also keeps its implementation linked from the static library.
	UObject::StaticClass();

	// Animation
	UAnimationAsset::StaticClass();
	UAnimDataModel::StaticClass();
	UAnimInstance::StaticClass();
	UAnimNotifyState::StaticClass();
	UAnimSequence::StaticClass();
	UAnimSequenceBase::StaticClass();
	UAnimSingleNodeInstance::StaticClass();
	UBlendSpace::StaticClass();
	UBlendSpace1D::StaticClass();
	UAnimGraph::StaticClass();
	USkeleton::StaticClass();

	// Asset
	UAsset::StaticClass();
	UAssetManager::StaticClass();

	// Camera
	ACameraActor::StaticClass();
	UCameraComponent::StaticClass();
	UCameraModifier_CameraShake::StaticClass();
	UCameraModifier_FOVTest::StaticClass();
	UCameraModifier::StaticClass();
	UCameraShakeBase::StaticClass();
	UCameraShakeHit::StaticClass();
	UCameraShakePattern::StaticClass();
	UCameraShakePatternWave::StaticClass();
	UCameraShakeSlam::StaticClass();
	UCameraShakeSourceComponent::StaticClass();
	APlayerCameraManager::StaticClass();

	// Component
	UActorComponent::StaticClass();
	UAmbientLightComponent::StaticClass();
	UBillboardComponent::StaticClass();
	UBoxComponent::StaticClass();
	UCapsuleComponent::StaticClass();
	UCharacterMovementComponent::StaticClass();
	UDecalComponent::StaticClass();
	UDirectionalLightComponent::StaticClass();
	UExponentialHeightFogComponent::StaticClass();
	UFXSystemComponent::StaticClass();
	ULightComponent::StaticClass();
	ULightComponentBase::StaticClass();
	UMeshComponent::StaticClass();
	UMovementComponent::StaticClass();
	UParticleSubUVComponent::StaticClass();
	UParticleSystemComponent::StaticClass();
	UPointLightComponent::StaticClass();
	UPrimitiveComponent::StaticClass();
	UProjectileMovementComponent::StaticClass();
	URotatingMovementComponent::StaticClass();
	USceneComponent::StaticClass();
	UScriptComponent::StaticClass();
	UShapeComponent::StaticClass();
	USkeletalMeshComponent::StaticClass();
	USkinnedMeshComponent::StaticClass();
	USphereComponent::StaticClass();
	USpotLightComponent::StaticClass();
	UStaticMeshComponent::StaticClass();
	UTextRenderComponent::StaticClass();

	// Engine
	UEngine::StaticClass();
	UGameEngine::StaticClass();
	UGameViewportClient::StaticClass();
	ULevel::StaticClass();
	UWorld::StaticClass();

	// Game
	ABullet::StaticClass();
	ACoin::StaticClass();
	ACoinLauncher::StaticClass();
	UCoinMovementComponent::StaticClass();
	ACoinRain::StaticClass();

	// GameFramework
	AActor::StaticClass();
	AAmbientLightActor::StaticClass();
	ABillboardActor::StaticClass();
	ADecalActor::StaticClass();
	ADirectionalLightActor::StaticClass();
	AEnemyActor::StaticClass();
	AExponentialHeightFogActor::StaticClass();
	AFakeSpotLightActor::StaticClass();
	ALuaTestActor::StaticClass();
	AParticleActor::StaticClass();
	AParticleEventManager::StaticClass();
	APlayerActor::StaticClass();
	APointLightActor::StaticClass();
	ARootMotionTestActor::StaticClass();
	ASkeletalMeshActor::StaticClass();
	ASpotLightActor::StaticClass();
	AStaticMeshActor::StaticClass();
	ATextRenderActor::StaticClass();
	ADefaultPawn::StaticClass();
	APawn::StaticClass();
	APlayerController::StaticClass();
	APlayerStart::StaticClass();
	USpringArmComponent::StaticClass();

	// Particle
	UParticleModuleAccelerationBase::StaticClass();
	UParticleModuleAccelerationConstant::StaticClass();
	UParticleModuleCollisionBase::StaticClass();
	UParticleModuleCollision::StaticClass();
	UParticleModuleEventBase::StaticClass();
	UParticleModuleEventGenerator::StaticClass();
	UParticleModuleColor::StaticClass();
	UParticleModuleColorOverLife::StaticClass();
	UParticleModuleLifeTime::StaticClass();
	UParticleModuleLocation::StaticClass();
	UParticleModuleLocationPrimitiveBox::StaticClass();
	UParticleModuleLocationPrimitiveSphere::StaticClass();
	UParticleModuleLocationPrimitiveCylinder::StaticClass();
	UParticleModuleRequired::StaticClass();
	UParticleModuleRotation::StaticClass();
	UParticleModuleRotationRate::StaticClass();
	UParticleModuleSize::StaticClass();
	UParticleModuleSizeScaleBySpeed::StaticClass();
	UParticleModuleSizeOfLifeTime::StaticClass();
	UParticleModuleSpawn::StaticClass();
	UParticleModuleTypeDataBase::StaticClass();
	UParticleModuleTypeDataBeam::StaticClass();
	UParticleModuleBeamSource::StaticClass();
	UParticleModuleBeamTarget::StaticClass();
	UParticleModuleTypeDataRibbon::StaticClass();
	UParticleModuleTypeDataMesh::StaticClass();
	UParticleModuleVelocity::StaticClass();
	UParticleEmitter::StaticClass();
	UParticleLODLevel::StaticClass();
	UParticleModule::StaticClass();
	UParticleSystem::StaticClass();

	// Render
	UMaterial::StaticClass();
	UMaterialInterface::StaticClass();
	UStaticMesh::StaticClass();
	USkeletalMesh::StaticClass();
	UTexture2D::StaticClass();

	// Sequence
	ULevelSceneSequencePlayer::StaticClass();
	ULevelSequence::StaticClass();
	ALevelSequenceActor::StaticClass();
	UMovieScene::StaticClass();
	UMovieScene3DTransformSection::StaticClass();
	UMovieScene3DTransformTrack::StaticClass();
	UMovieSceneCameraCutSection::StaticClass();
	UMovieSceneCameraCutTrack::StaticClass();
	UMovieSceneFadeSection::StaticClass();
	UMovieSceneFadeTrack::StaticClass();
	UMovieScenePropertyTrack::StaticClass();
	UMovieSceneSection::StaticClass();
	UMovieSceneSequence::StaticClass();
	UMovieSceneSequencePlayer::StaticClass();

	// Text
	UFont::StaticClass();
}
