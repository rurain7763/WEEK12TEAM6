#pragma once

#include "Math/Frustum.h"
#include "Math/Color.h"
#include "Render/MeshRenderData.h"
#include "Render/Material.h"
#include "Render/Buffer.h"
#include "Render/RenderPacket.h"
#include <d3d11.h>
#include <wrl/client.h>
class FScene;
class FConstantBuffer;
class UPrimitiveComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UStaticMesh;
class USkeletalMesh;
class UMaterial;
class UParticleSystemComponent;
class FRenderQueue;
struct FViewBasis;

enum EObjectMaskBit
{
    OM_CastShadow = 0x1,
    OM_ReceiveShadow = 0x2,
    OM_ReceiveDecals = 0x4
};

class FPrimitiveSceneProxy
{
public:
    explicit FPrimitiveSceneProxy(UPrimitiveComponent* InComponent);
    virtual ~FPrimitiveSceneProxy() = default;

    void UpdateTransform();
    virtual void UpdateRenderState();
    void OnMeshRenderDataChanged();
    virtual void OnMeshDestroyed();
    virtual void SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewBasis& View) {}
    // Expose resources; the renderer selects shader stages and binding slots.
    virtual ID3D11ShaderResourceView* GetBoneMatricesSRV() const { return nullptr; }
    virtual ID3D11Buffer* GetBoneWeightConstantBuffer() const { return nullptr; }

    UPrimitiveComponent* GetComponent() const { return Component; }

    const FAABB& GetBounds() const { return Bounds; }
    const FMatrix& GetLocalToWorld() const { return LocalToWorld; }
    const FMatrix& GetWorldToLocal() const { return WorldToLocal; }
    // 노멀 변환용 World 역전치. Transform이 바뀔 때만 다시 계산한다.
    const FMatrix& GetNormalMatrix() const { return NormalMatrix; }
    const FLODSphere& GetLODSphere() const { return LODSphere; }
    bool IsVisible() const { return bVisible; }
    bool IsCastShadow() const { return bCastShadow; }
    bool IsReceiveShadow() const { return bReceiveShadow; }
    bool ReceivesDecals() const{ return bReceivesDecals; }
    bool IsHiddenInGame() const { return bHiddenInGame; }
    uint32 GetObjectMask() const { return ObjectMask; }

    uint8 GetLODCount() const { return RenderState ? RenderState->LODCount : 1; }
    const FMeshRenderState* GetRenderState() const { return RenderState.get(); }
    bool IsUsingSkinning() const { return RenderState && RenderState->bUseSkinning; }
    const float* GetLODThresholdsSq() const { return RenderState->LODThresholdSq; }
    const FCachedMeshLOD& GetLOD(uint32 Index) const { return RenderState->LODs[Index]; }
    const FCachedMeshSection& GetSection(uint32 Index) const { return RenderState->Sections[Index]; }
    FScene* GetScene() const { return Scene; }

protected:
    friend class FScene;

    virtual FBox GetLocalBounds() const;
    virtual FLODSphere GetLocalLODSphere() const;

protected:
    UPrimitiveComponent* Component;
    TSharedPtr<FMeshRenderState> RenderState;

private:
    FScene* Scene = nullptr;
    FMatrix LocalToWorld;
    FMatrix WorldToLocal;
    FMatrix NormalMatrix;
    FAABB Bounds;
    FLODSphere LODSphere;
    int32 PackedIndex = INDEX_NONE;
    bool bQueuedForUpdate = false;
    bool bRenderStateQueued = false;
    bool bVisible = true;
    bool bCastShadow = true;
    bool bReceiveShadow = true;
    bool bReceivesDecals = true;
    bool bHiddenInGame = false;
    uint32 ObjectMask = 0;
};

class FStaticMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
    FStaticMeshSceneProxy(UStaticMeshComponent* InComponent);
    ~FStaticMeshSceneProxy() override;

    void UpdateRenderState() override;
    void OnMeshDestroyed() override;

protected:
    FBox GetLocalBounds() const override;
    FLODSphere GetLocalLODSphere() const override;

private:
    UStaticMesh* Mesh = nullptr;
};

class FSkeletalMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
    FSkeletalMeshSceneProxy(USkeletalMeshComponent* InComponent);
    ~FSkeletalMeshSceneProxy() override;
    ID3D11Buffer* GetBoneWeightConstantBuffer() const override;

    void UpdateRenderState() override;
    void OnMeshDestroyed() override;
    bool UpdateBoneMatrixBuffer();

    ID3D11ShaderResourceView* GetBoneMatricesSRV() const override
    {
        return BoneMatrixSRV.Get();
    }

protected:
    FBox GetLocalBounds() const override;
    FLODSphere GetLocalLODSphere() const override;

private:
    USkeletalMesh* Mesh = nullptr;

    Microsoft::WRL::ComPtr<ID3D11Buffer> BoneMatrixBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> BoneMatrixSRV;
    uint32 BoneMatrixCapacity = 0;

    TUniquePtr<FConstantBuffer> BoneWeightCB;
    bool bUploadedBoneWeightEnabled = false;
    int32 UploadedHeatmapBoneIndex = -1;
};

struct FParticleSpriteRenderData
{
    FVector Location;
    FVector Size;
    float Rotation;
    FLinearColor TintColor = FLinearColor::White;
};
enum class EDynamicEmitterType 
{
    Sprite,
    Mesh,
    Beam,
    Ribbon,
};
struct FParticleStripRenderData
{
    TArray<FVector> Points;
    float Width = 1.0f;
    FLinearColor Color = FLinearColor::White;
};

struct FParticleEmitterRenderData
{
    FMatrix SimulateToWorld;
    UMaterial* Material = nullptr;
    TArray<FParticleSpriteRenderData> Particles;
    EDynamicEmitterType DynamicEmitterType = EDynamicEmitterType::Sprite;
    TArray<FParticleStripRenderData> Strips;
    UStaticMesh* mStaticMesh{};
    TArray<FObjectInstance> mMeshParticles{};
};

class FParticleSystemSceneProxy : public FPrimitiveSceneProxy
{
public:
    FParticleSystemSceneProxy(UParticleSystemComponent* InComponent);
    ~FParticleSystemSceneProxy() override;

    void SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewBasis& View);

    void SetDynamicData(const TArray<FParticleEmitterRenderData>& InEmitterRenderDatas);

protected:
    virtual FBox GetLocalBounds() const;
    virtual FLODSphere GetLocalLODSphere() const;

private:
    void SubmitMeshParticles(FRenderQueue& RenderQueue, const FViewBasis& View) const;

private:
    TArray<FParticleEmitterRenderData> EmitterRenderDatas;
    UStaticMesh* QuadMesh = nullptr;
    FBox LocalBounds;

    TUniquePtr<FVertexBuffer> StripVB;
    TUniquePtr<FIndexBuffer> StripIB;
    uint32 StripVertexCapacity = 0;
    uint32 StripIndexCapacity = 0;
};

