#pragma once
#include "Math/Transform.h"
#include "Math/Color.h"
#include "Containers/Array.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include <array>
#include <utility>

class FPrimitiveSceneProxy;
inline constexpr uint32 InvalidObjectConstantSlot = ~0u;

struct FGeometryBinding
{
    FVertexBuffer* VertexBuffers[2];
    int32 VertexBufferCount = 0;
    FIndexBuffer* IndexBuffer = nullptr;
    uint32 StartIndex = 0;
    uint32 IndexCount = 0; // Zero selects the full index buffer.

    bool operator!=(const FGeometryBinding& Other)
    {
        return StartIndex != Other.StartIndex || IndexCount != Other.IndexCount || !HasSameBuffers(Other.VertexBuffers, Other.VertexBufferCount, Other.IndexBuffer);
    }

    void SetVertexBuffers(FVertexBuffer* const* InVertexBuffers, int32 InVertexBufferCount)
    {
        std::memcpy(VertexBuffers, InVertexBuffers, sizeof(FVertexBuffer*) * InVertexBufferCount);
        VertexBufferCount = InVertexBufferCount;
    }

    bool HasSameBuffers(FVertexBuffer* const* InVertexBuffers, int32 InVertexBufferCount, const FIndexBuffer* InIndexBuffer) const
    {
        return InVertexBufferCount == VertexBufferCount && IndexBuffer == InIndexBuffer && std::equal(VertexBuffers, VertexBuffers + VertexBufferCount, InVertexBuffers);
    }
};

struct FRenderPacket
{
    const FPrimitiveSceneProxy* Proxy = nullptr;
    const FMatrix* Model = nullptr;
    const FMatrix* NormalMatrix = nullptr;
    FGeometryBinding Geometry;
    UMaterial* Material = nullptr;
    FLinearColor ColorMultiplier = FLinearColor::White;
    const void* MaterialParamData = nullptr;
    float CameraDistanceSquared = 0.0f;
    uint32 MaterialParamDataSize = 0;
    uint32 ObjectConstantSlot = InvalidObjectConstantSlot;
    bool bOccludedByGpu = false;
};

// Shared buffers, material and index range; one ordinary indexed draw per item.
struct FMeshDrawItem
{
    const FPrimitiveSceneProxy* Proxy;
    uint32 ObjectConstantSlot;
    uint32 bOccludedByGpu;
};

struct FObjectInstance
{
    FMatrix Model;
    FMatrix NormalMatrix;
    FLinearColor Color = FLinearColor::White;
    uint32 ObjectMask = 0;
    uint32 InstancePadding[3] = {};
};

struct FMeshDrawGroup
{
    bool bUseSkinning = false;
    FGeometryBinding Geometry;
    UMaterial* Material = nullptr;
    TArray<FMeshDrawItem> Items;
    TArray<FObjectInstance> Instances;
};

// Owns the current frame's billboard/particle matrices together with their packets.
// Blocks keep matrix addresses stable when either array grows.
class FRenderQueue : public TArray<FRenderPacket>
{
    using FPackets = TArray<FRenderPacket>;
    static constexpr uint32 MatricesPerBlock = 256;
    using FMatrixBlock = std::array<FMatrix, MatricesPerBlock>;
    TArray<TUniquePtr<FMatrixBlock>> MatrixBlocks;
    uint32 MatrixCount = 0;

public:
    FRenderQueue() = default;
    FRenderQueue(const FRenderQueue&) = delete;
    FRenderQueue& operator=(const FRenderQueue&) = delete;
    FRenderQueue(FRenderQueue&& Other) noexcept
        : FPackets(std::move(Other)), MatrixBlocks(std::move(Other.MatrixBlocks)),
          MatrixCount(std::exchange(Other.MatrixCount, 0)) 
    {
    }

    FRenderQueue& operator=(FRenderQueue&& Other) noexcept
    {
        if (this != &Other)
        {
            FPackets::operator=(std::move(Other));
            MatrixBlocks = std::move(Other.MatrixBlocks);
            MatrixCount = std::exchange(Other.MatrixCount, 0);
        }
        return *this;
    }

    const FMatrix* StoreWorldMatrix(const FMatrix& World)
    {
        const uint32 BlockIndex = MatrixCount / MatricesPerBlock;
        if (BlockIndex >= static_cast<uint32>(MatrixBlocks.Num()))
            MatrixBlocks.Add(MakeUnique<FMatrixBlock>());
        FMatrix& Stored = (*MatrixBlocks[BlockIndex])[MatrixCount % MatricesPerBlock];
        Stored = World;
        ++MatrixCount;
        return &Stored;
    }

    void Reset()
    {
        FPackets::Reset();
        MatrixCount = 0;
    }
};
