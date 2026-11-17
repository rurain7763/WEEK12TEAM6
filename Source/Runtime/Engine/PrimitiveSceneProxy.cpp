#include "EnginePCH.h"
#include "PrimitiveSceneProxy.h"
#include "Component/PrimitiveComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Component/SkeletalMeshComponent.h"
#include "Render/RenderCommand.h"
#include "Render/Mesh.h"
#include "Render/Renderer.h"
#include "Engine/Scene.h"
#include "Render/Buffer.h"
#include "Core/Stats/SkinningStats.h"
#include "Component/ParticleSystemComponent.h"
#include "Asset/AssetManager.h"
#include "Camera/CameraTypes.h"
#include "Render/RenderPacket.h"

FPrimitiveSceneProxy::FPrimitiveSceneProxy(UPrimitiveComponent* InComponent)
    : Component(InComponent)
{
}

void FPrimitiveSceneProxy::OnMeshRenderDataChanged()
{
    if (Scene)
    {
        Scene->MarkRenderStateDirty(this);
        Scene->MarkDirty(this);
    }
}

void FPrimitiveSceneProxy::OnMeshDestroyed()
{
    RenderState.reset();
}

FBox FPrimitiveSceneProxy::GetLocalBounds() const
{
    return Component->CalcLocalBounds();
}

FLODSphere FPrimitiveSceneProxy::GetLocalLODSphere() const
{
    return FLODSphere();
}

void FPrimitiveSceneProxy::UpdateTransform()
{
    LocalToWorld = GetComponent()->GetWorldMatrix();
    WorldToLocal = LocalToWorld.Inverse();
    NormalMatrix = MakeNormalMatrix(LocalToWorld);

    const FBox LocalBounds = GetLocalBounds();
    Bounds = MakeWorldBounds(LocalBounds.GetWorldAABB(LocalToWorld));

    const FLODSphere LocalSphere = GetLocalLODSphere();
    LODSphere = LocalSphere.Transform(LocalToWorld);
}

void FPrimitiveSceneProxy::UpdateRenderState()
{
    RenderState = MakeShared<FMeshRenderState>();

    UPrimitiveComponent* Component = GetComponent();
    bVisible = Component->IsVisible();
    bCastShadow = Component->IsCastShadow();
    bReceiveShadow = Component->IsReceiveShadow();
    bReceivesDecals = Component->ReceivesDecals();
    bHiddenInGame = Component->IsHiddenInGame();

    ObjectMask = 0;
    if (bCastShadow)
    {
        ObjectMask |= OM_CastShadow;
    }

    if (bReceiveShadow)
    {
        ObjectMask |= OM_ReceiveShadow;
    }

    if (bReceivesDecals)
    {
        ObjectMask |= OM_ReceiveDecals;
    }
}

FStaticMeshSceneProxy::FStaticMeshSceneProxy(UStaticMeshComponent* InComponent)
    : FPrimitiveSceneProxy(InComponent)
{
}

FStaticMeshSceneProxy::~FStaticMeshSceneProxy()
{
    if (!Mesh)
    {
        return;
    }

    Mesh->UnregisterProxy(this);
}

void FStaticMeshSceneProxy::UpdateRenderState()
{
    FPrimitiveSceneProxy::UpdateRenderState();

    UStaticMeshComponent* ActualComponent = Cast<UStaticMeshComponent>(Component);

    UStaticMesh* PreviousMesh = Mesh;
    Mesh = ActualComponent->GetStaticMesh();

    if (Mesh != PreviousMesh)
    {
        if (PreviousMesh)
        {
            PreviousMesh->UnregisterProxy(this);
        }

        if (Mesh)
        {
            Mesh->RegisterProxy(this);
        }

        if (GetScene())
        {
            GetScene()->MarkDirty(this);
        }
    }

    if (!Mesh)
    {
        return;
    }

    auto State = MakeShared<FMeshRenderState>();
    State->bUseSkinning = false;
    State->LODCount = static_cast<uint8>(std::min(Mesh->GetLODCount(), 4u));
    for (uint32 I = 0; I < 3; ++I)
    {
        State->LODThresholdSq[I] = Mesh->ScreenThresholds[I] * Mesh->ScreenThresholds[I];
    }

    for (uint32 LODIndex = 0; LODIndex < State->LODCount; ++LODIndex)
    {
        FCachedMeshLOD& LOD = State->LODs[LODIndex];
        LOD.VertexBuffers[0] = Mesh->GetVertexBuffer(LODIndex);
        LOD.VertexBufferCount = 1;
        LOD.IndexBuffer = Mesh->GetIndexBuffer(LODIndex);
        LOD.FirstSection = State->Sections.Num();

        for (const FMeshSection& Section : Mesh->GetMeshData(LODIndex).Sections)
        {
            UMaterial* Material = Component->GetMaterial(Section.MaterialSlotIndex);
            if (Material)
            {
                State->Sections.Add({ Material, Section.StartIndex, Section.IndexCount });
            }
        }

        LOD.NumSections = State->Sections.Num() - LOD.FirstSection;
    }

    RenderState = State;
}

void FStaticMeshSceneProxy::OnMeshDestroyed()
{
    Mesh = nullptr;
    FPrimitiveSceneProxy::OnMeshDestroyed();
}

FBox FStaticMeshSceneProxy::GetLocalBounds() const
{
    return Mesh ? Mesh->GetRenderBounds() : FPrimitiveSceneProxy::GetLocalBounds();
}

FLODSphere FStaticMeshSceneProxy::GetLocalLODSphere() const
{
    return Mesh ? Mesh->GetLocalLODSphere() : FPrimitiveSceneProxy::GetLocalLODSphere();
}

FSkeletalMeshSceneProxy::FSkeletalMeshSceneProxy(USkeletalMeshComponent* InComponent)
    : FPrimitiveSceneProxy(InComponent)
{
}

FSkeletalMeshSceneProxy::~FSkeletalMeshSceneProxy()
{
}

ID3D11Buffer* FSkeletalMeshSceneProxy::GetBoneWeightConstantBuffer() const
{
    return BoneWeightCB ? BoneWeightCB->GetBuffer() : nullptr;
}

bool FSkeletalMeshSceneProxy::UpdateBoneMatrixBuffer()
{
    SCOPE_CYCLE_COUNTER(STAT_SkinningGPUBoneUpload);

    const auto* ActualComponent = Cast<USkeletalMeshComponent>(GetComponent());
    const TArray<FMatrix>& BoneMatrices = ActualComponent->GetSkinningMatrices();
    const uint32 Count = BoneMatrices.Num();
    if (Count == 0)
    {
        BoneMatrixSRV.Reset();
        BoneMatrixBuffer.Reset();
        BoneMatrixCapacity = 0;
        return true;
    }

    ID3D11Device* Device = RenderCommand::GetDevice();
    if (!BoneMatrixBuffer || !BoneMatrixSRV || Count > BoneMatrixCapacity)
    {
        D3D11_BUFFER_DESC BufferDesc{};
        BufferDesc.ByteWidth = Count * sizeof(FMatrix);
        BufferDesc.Usage = D3D11_USAGE_DEFAULT;
        BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        BufferDesc.StructureByteStride = sizeof(FMatrix);

        Microsoft::WRL::ComPtr<ID3D11Buffer> NewBuffer;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> NewSRV;
        HRESULT Result = Device->CreateBuffer(&BufferDesc, nullptr, NewBuffer.GetAddressOf());
        if (FAILED(Result))
        {
            return false;
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
        SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        SRVDesc.Buffer.NumElements = Count;
        Result = Device->CreateShaderResourceView(NewBuffer.Get(), &SRVDesc, NewSRV.GetAddressOf());
        if (FAILED(Result))
        {
            return false;
        }

        BoneMatrixBuffer = std::move(NewBuffer);
        BoneMatrixSRV = std::move(NewSRV);
        BoneMatrixCapacity = Count;
    }

    ID3D11DeviceContext* Context = RenderCommand::GetContext();
    const D3D11_BOX UploadRange{ 0, 0, 0, static_cast<UINT>(Count * sizeof(FMatrix)), 1, 1 };
    Context->UpdateSubresource(BoneMatrixBuffer.Get(), 0, &UploadRange, BoneMatrices.GetData(), 0, 0);

    FSkinningStats::AddGPUMesh(static_cast<int32>(Count), static_cast<int64>(Count) * sizeof(FMatrix));
    return true;
}

void FSkeletalMeshSceneProxy::UpdateRenderState()
{
    FPrimitiveSceneProxy::UpdateRenderState();

    auto* ActualComponent = Cast<USkeletalMeshComponent>(GetComponent());
    Mesh = ActualComponent->GetSkeletalMesh();

    const bool bCPUSkin = ActualComponent->IsCPUSkinningEnabled()
        && ActualComponent->GetCPUSkinVertexBuffer();

    // CPU 스킨이면 본 행렬 업로드를 건너뛴다
    if (!bCPUSkin && !UpdateBoneMatrixBuffer())
    {
        RenderState.reset();
        return;
    }

    if (!Mesh)
    {
        return;
    }

    struct FBoneWeightConstants
    {
        uint32 Enabled;
        int32 BoneIndex;
        uint32 Padding[2];
    };
    static_assert(sizeof(FBoneWeightConstants) == 16);

    const bool bEnabled = ActualComponent->IsBoneWeightVisualizationEnabled();
    const int32 BoneIndex = ActualComponent->GetHeatmapBoneIndex();

    if (!BoneWeightCB || bEnabled != bUploadedBoneWeightEnabled || BoneIndex != UploadedHeatmapBoneIndex)
    {
        if (!BoneWeightCB)
        {
            BoneWeightCB = RenderCommand::CreateConstantBuffer(sizeof(FBoneWeightConstants));
        }

        if (BoneWeightCB && BoneWeightCB->GetBuffer())
        {
            const FBoneWeightConstants Constants{ bEnabled ? 1u : 0u, BoneIndex,{} };
            RenderCommand::UpdateBufferData(BoneWeightCB.get(), &Constants);

            bUploadedBoneWeightEnabled = bEnabled;
            UploadedHeatmapBoneIndex = BoneIndex;
        }
        else
        {
            BoneWeightCB.reset();
        }
    }

    RenderState->LODCount = 1;
    RenderState->bUseSkinning = !bCPUSkin;

    const FModelMeshData& ModelMeshData = Mesh->GetMeshData();

    for (int32 i = 0; i < ModelMeshData.Sections.Num(); i++)
    {
        const FMeshSection& Section = ModelMeshData.Sections[i];

        FCachedMeshSection& CachedMeshSection = RenderState->Sections.AddDefaulted_GetRef();
        CachedMeshSection.Material = ActualComponent->GetMaterial(Section.MaterialSlotIndex);
        CachedMeshSection.StartIndex = Section.StartIndex;
        CachedMeshSection.IndexCount = Section.IndexCount;
    }

    FCachedMeshLOD& LOD = RenderState->LODs[0];

    if (bCPUSkin)
    {
        LOD.VertexBuffers[0] = ActualComponent->GetCPUSkinVertexBuffer();
        // CPU 스키닝에서는 스키닝이 이미 수행되어서 스킨 가중치를 넘길 필요 X
        LOD.VertexBuffers[1] = nullptr;
        LOD.VertexBufferCount = 1;
    }
    else
    {
        LOD.VertexBuffers[0] = Mesh->GetVertexBuffer();
        LOD.VertexBuffers[1] = Mesh->GetSkinWeightBuffer();
        LOD.VertexBufferCount = 2;
    }
    LOD.IndexBuffer = Mesh->GetIndexBuffer();
    LOD.FirstSection = 0;
    LOD.NumSections = ModelMeshData.Sections.Num();
}

void FSkeletalMeshSceneProxy::OnMeshDestroyed()
{
    Mesh = nullptr;
    BoneMatrixSRV.Reset();
    BoneMatrixBuffer.Reset();
    BoneMatrixCapacity = 0;
    FPrimitiveSceneProxy::OnMeshDestroyed();
}

FBox FSkeletalMeshSceneProxy::GetLocalBounds() const
{
	// ����� �ִϸ��̼��� ���� ���� �ٿ�� �ڽ��� �״�� ����. �ִϸ��̼��� ������ �� ������ ����ؾ� �Ѵ�.
	if (!Mesh)
	{
		return FPrimitiveSceneProxy::GetLocalBounds();
	}

    const FModelMeshData& ModelMeshData = Mesh->GetMeshData();
    return ModelMeshData.AABB;
}

FLODSphere FSkeletalMeshSceneProxy::GetLocalLODSphere() const
{
    return FLODSphere();
}

FParticleSystemSceneProxy::FParticleSystemSceneProxy(UParticleSystemComponent* InComponent)
    : FPrimitiveSceneProxy(InComponent)
{
    QuadMesh = UAssetManager::GetAssetByPath<UStaticMesh>("Quad");
}

FParticleSystemSceneProxy::~FParticleSystemSceneProxy()
{
}

void FParticleSystemSceneProxy::SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewBasis& View)
{
    // Strip(Beam,Ribbon) 처리
    uint32 NumStripVertices = 0;
    uint32 NumStripIndices = 0;
    for (const FParticleEmitterRenderData& Emitter : EmitterRenderDatas)
    {
        if ((Emitter.DynamicEmitterType != EDynamicEmitterType::Beam && Emitter.DynamicEmitterType != EDynamicEmitterType::Ribbon) || !Emitter.Material) continue;

        for (const FParticleStripRenderData& Strip : Emitter.Strips)
        {
            const uint32 N = Strip.Points.Num();
            if (N < 2) continue;
            NumStripVertices += N * 2; // 점하나 -> 위아래정점2개
            NumStripIndices += (N - 1) * 6; //칸하나 -> 삼각형2개 -> 인덱스 6개
        }
    }
    //크기부족일때 재할당
    if (NumStripVertices > StripVertexCapacity)
    {
        StripVB = RenderCommand::CreateDynamicVertexBuffer(NumStripVertices * sizeof(FParticleVertex), sizeof(FParticleVertex));
        StripVertexCapacity = NumStripVertices;
    }
    if (NumStripIndices > StripIndexCapacity)
    {
        StripIB = RenderCommand::CreateDynamicIndexBuffer(NumStripIndices);
        StripIndexCapacity = NumStripIndices;
    }
    // Beam을 삼각형화 해서 띠 모양으로 정점변환단계
    if (NumStripVertices > 0)
    {
        FParticleVertex* V = static_cast<FParticleVertex*>(RenderCommand::MapWriteDiscard(StripVB.get()));
        uint32* I = static_cast<uint32*>(RenderCommand::MapWriteDiscard(StripIB.get()));
        uint32 VertexCursor = 0;   // 지금까지 쓴 정점 수
        uint32 IndexCursor = 0;   // 지금까지 쓴 인덱스 수

        for (const FParticleEmitterRenderData& Emitter : EmitterRenderDatas)
        {
            if ((Emitter.DynamicEmitterType != EDynamicEmitterType::Beam && Emitter.DynamicEmitterType != EDynamicEmitterType::Ribbon) || !Emitter.Material) continue;

            for (const FParticleStripRenderData& Strip : Emitter.Strips)
            {
                const TArray<FVector>& P = Strip.Points;
                const uint32 N = P.Num();
                if (N < 2) continue;

                const uint32 Base = VertexCursor;   // 이 띠의 첫 정점 번호
                const uint32 FirstIndex = IndexCursor;    // 이 띠의 첫 인덱스 위치 (4-4에서 씀)

                // (가) 정점: 점마다 위·아래 2개
                for (uint32 i = 0; i < N; ++i)
                {
                    FVector Dir = (i + 1 < N) ? P[i + 1] - P[i] : P[i] - P[i - 1];
                    FVector Side = FVector::Cross(Dir, View.Location - P[i]);
                    if (Side.SizeSquared() < 1e-8f) Side = FVector::Cross(Dir, FVector(0, 0, 1));
                    if (Side.SizeSquared() < 1e-8f) Side = FVector::Cross(Dir, FVector(0, 1, 0));
                    Side = Side.Normalized() * (Strip.Width * 0.5f);
                    const float U = static_cast<float>(i) / (N - 1);

                    FParticleVertex& Top = V[Base + 2 * i];
                    FParticleVertex& Bot = V[Base + 2 * i + 1];
                    Top = FParticleVertex();  Top.Position = P[i] + Side;  Top.UV = FVector2(U, 0.f);
                    Bot = FParticleVertex();  Bot.Position = P[i] - Side;  Bot.UV = FVector2(U, 1.f);
                }

                // (나) 인덱스: 칸마다 삼각형 2개
                for (uint32 i = 0; i + 1 < N; ++i)
                {
                    const uint32 A = Base + 2 * i;
                    I[IndexCursor++] = A;      I[IndexCursor++] = A + 2;  I[IndexCursor++] = A + 1;
                    I[IndexCursor++] = A + 1;  I[IndexCursor++] = A + 2;  I[IndexCursor++] = A + 3;
                }
                VertexCursor += N * 2;

                FRenderPacket& Packet = RenderQueue.AddDefaulted_GetRef();
                Packet.Proxy = this;
                Packet.Geometry.VertexBuffers[0] = StripVB.get();
                Packet.Geometry.VertexBufferCount = 1;
                Packet.Geometry.IndexBuffer = StripIB.get();
                Packet.Geometry.StartIndex = FirstIndex;                 // 이 띠의 첫 인덱스
                Packet.Geometry.IndexCount = IndexCursor - FirstIndex;   // 이 띠의 인덱스 수 (60)
                Packet.Material = Emitter.Material;
                Packet.ColorMultiplier = Strip.Color;
                // Packet.Model은 비워 둠 → 렌더러가 단위 행렬 사용 (정점이 이미 월드 좌표)
                Packet.CameraDistanceSquared = (View.Location - P[N / 2]).SizeSquared();
            }
        }

        RenderCommand::Unmap(StripVB.get());
        RenderCommand::Unmap(StripIB.get());
    }

    // 일반 Spirte Particle Render 
    if (!QuadMesh)
    {
        return;
    }

    const int32 LODIndex = 0;
    FVertexBuffer* VB = QuadMesh->GetVertexBuffer(LODIndex);
    FIndexBuffer* IB = QuadMesh->GetIndexBuffer(LODIndex);

    for (int32 EmitterIndex = 0; EmitterIndex < EmitterRenderDatas.Num(); ++EmitterIndex)
    {
        const FParticleEmitterRenderData& Emitter = EmitterRenderDatas[EmitterIndex];
        if (!Emitter.Material)
        {
            continue;
        }
        if (Emitter.DynamicEmitterType == EDynamicEmitterType::Beam || Emitter.DynamicEmitterType == EDynamicEmitterType::Ribbon)
        {
        continue;
        }

        for (int32 ParticleIndex = 0; ParticleIndex < Emitter.Particles.Num(); ++ParticleIndex)
        {
            const FParticleSpriteRenderData& Sprite = Emitter.Particles[ParticleIndex];

            FVector ToCamera = View.Location - Sprite.Location;
            FMatrix BillboardMatrix = View.BuildBillboardMatrix(Sprite.Location, Sprite.Size.X, Sprite.Size.Y, Sprite.Rotation);

            FRenderPacket& Packet = RenderQueue.AddDefaulted_GetRef();
            Packet.Proxy = this;
            Packet.Geometry.VertexBuffers[0] = VB;
            Packet.Geometry.VertexBufferCount = 1;
            Packet.Geometry.IndexBuffer = IB;
            Packet.Geometry.StartIndex = 0;
            Packet.Geometry.IndexCount = IB->GetIndexCount();
            Packet.Material = Emitter.Material;
            Packet.ColorMultiplier = Sprite.MaterialParams.TintColor;
            Packet.Model = RenderQueue.StoreWorldMatrix(BillboardMatrix);
            Packet.CameraDistanceSquared = ToCamera.SizeSquared();
        }
    }
}

void FParticleSystemSceneProxy::SetDynamicData(const TArray<FParticleEmitterRenderData>& InEmitterRenderDatas)
{
    LocalBounds = FBox();
    LocalBounds.Min = FVector(FLT_MAX);
    LocalBounds.Max = FVector(-FLT_MAX);

    bool bNoParticle = true;

    const FBox& LocalBound = QuadMesh->GetRenderBounds();

    float MaxPaddingSize = FMath::Max(LocalBound.Min.GetAbs().MaxComponent(), LocalBound.Max.GetAbs().MaxComponent());

    EmitterRenderDatas.SetNum(InEmitterRenderDatas.Num());
    for (int32 i = 0; i < InEmitterRenderDatas.Num(); i++)
    {
        const FParticleEmitterRenderData& SrcEmitterRenderData = InEmitterRenderDatas[i];
        const TArray<FParticleSpriteRenderData>& SrcParticles = SrcEmitterRenderData.Particles;
        
        FParticleEmitterRenderData& DstEmitterRenderData = EmitterRenderDatas[i];
        TArray<FParticleSpriteRenderData>& DstParticles = DstEmitterRenderData.Particles;

        // 에미터 복제
        DstEmitterRenderData = SrcEmitterRenderData;

        DstParticles.SetNum(SrcParticles.Num());
        for (int32 j = 0; j < SrcParticles.Num(); j++)
        {
            const FParticleSpriteRenderData& SrcParticle = SrcParticles[j];
            FParticleSpriteRenderData& DstParticle = DstParticles[j];

            // 파티클 복제
            DstParticle = SrcParticle;

            // AABB 갱신
            float MaxSizeComponent = DstParticle.Size.GetAbs().MaxComponent();
            FVector BoundSize = FVector(MaxPaddingSize * MaxSizeComponent * 2.f);

            LocalBounds.Expand(DstParticle.Location + BoundSize);
            LocalBounds.Expand(DstParticle.Location - BoundSize);

            bNoParticle = false;
        }

        //띠루프 추가
        for (const FParticleStripRenderData& Strip : DstEmitterRenderData.Strips)
        {
            const FVector Pad(Strip.Width);
            for (const FVector& P : Strip.Points)
            {
                LocalBounds.Expand(P + Pad);
                LocalBounds.Expand(P - Pad);
            }
            if (Strip.Points.Num() > 0) bNoParticle = false;
        }

    }

    if (bNoParticle)
    {
        LocalBounds = FBox();
    }
    else
    {
        // 월드 AABB -> 로컬 AABB
        FMatrix WorldMatrix = Component->GetWorldMatrix();
        LocalBounds = LocalBounds.GetWorldAABB(WorldMatrix.Inverse());
    }

    GetScene()->MarkDirty(this);
}

FBox FParticleSystemSceneProxy::GetLocalBounds() const
{
    return LocalBounds;
}

FLODSphere FParticleSystemSceneProxy::GetLocalLODSphere() const
{
    return FLODSphere();
}
