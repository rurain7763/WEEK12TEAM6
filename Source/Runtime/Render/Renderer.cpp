#include "EnginePCH.h"
#include "Renderer.h"
#include "ColorWorkflow.h"
#include "Core/EngineTimer.h"
#include "Shader.h"
#include "Mesh.h"

#include "Core/Stats/LightweightStats.h"

#include "Engine/PrimitiveSceneProxy.h"

#include "RenderCommand.h"
#include "RenderResourceManager.h"
#include "Asset/ModelData.h"

#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <bit>

#include "Engine/LightSceneProxy.h"
#include "Component/SkeletalMeshComponent.h"
#include "Component/ParticleSystemComponent.h"
#include "Camera/CameraTypes.h"
#include <map>
#include <tuple>
#include <Core/StatOverlay.h>
#include "Core/Stats/ParticleStats.h"

namespace
{
	// 파티클 stat의 드로우콜 집계용
	bool IsParticleProxy(const FPrimitiveSceneProxy* Proxy)
	{
		return dynamic_cast<const FParticleSystemSceneProxy*>(Proxy) != nullptr;
	}
}

DECLARE_CYCLE_STAT("Draw Render Packets", STAT_DrawRenderPackets);
DECLARE_CYCLE_STAT("Render Queue Sorting", STAT_RenderQueueSorting);
DECLARE_CYCLE_STAT("Upload Per-Object CB", STAT_UploadPerObjectCB);
DECLARE_CYCLE_STAT("Shadow Pass (Total)", STAT_Shadow);

namespace
{
	// VSSetConstantBuffers1의 오프셋은 16개 상수(256바이트) 단위여야 하므로 오브젝트마다 256바이트 칸을 쓴다.
	constexpr uint32 PerObjectSlotConstants = 16;

	constexpr uint32 MinPerObjectSlots = 1024;

	bool HasBoundNormalMap(const UMaterial* Material)
	{
		const FTexture2D* Resource = Material->Textures.Num() > 1 && Material->Textures[1]
			? Material->Textures[1]->GetResource() : nullptr;
		return Resource && Resource->GetRawSRV() != nullptr;
	}

	// Surface shaders use VS t13 for bones and VS/PS b8 for weight visualization.
	void BindSurfaceSkinningResources(const FPrimitiveSceneProxy* Proxy)
	{
		ID3D11ShaderResourceView* Bones = Proxy ? Proxy->GetBoneMatricesSRV() : nullptr;
		RenderCommand::BindShaderResource(13, Bones, EShaderBindFlagBits::Vertex);

		ID3D11Buffer* Weights = Proxy ? Proxy->GetBoneWeightConstantBuffer() : nullptr;

		auto* Context = RenderCommand::GetContext();
		Context->VSSetConstantBuffers(8, 1, &Weights);
		Context->PSSetConstantBuffers(8, 1, &Weights);
	}

	const FMatrix& GetPacketWorld(const FRenderPacket& Packet)
	{
		return Packet.Model ? *Packet.Model : FMatrix::Identity;
	}

	FMatrix GetPacketNormalMatrix(const FRenderPacket& Packet)
	{
		if (Packet.NormalMatrix) return *Packet.NormalMatrix;
		return MakeNormalMatrix(GetPacketWorld(Packet));
	}

	using FStaticDrawKey = std::tuple<UMaterial*, FVertexBuffer*, FIndexBuffer*, uint32, uint32>;
	using FStaticDrawLookup = std::map<FStaticDrawKey, int32>;

	bool IsSkeletalProxy(const FPrimitiveSceneProxy* Proxy) {
		return Proxy && (Proxy->IsUsingSkinning() || Cast<USkeletalMeshComponent>(Proxy->GetComponent()));
	}

	bool IsStaticInstanceGeometry(const FGeometryBinding& Geometry) {
		return Geometry.VertexBufferCount == 1 && Geometry.VertexBuffers[0] && Geometry.VertexBuffers[0]->GetStride() == sizeof(FVertexPNCT) && Geometry.IndexBuffer && Geometry.IndexCount > 0;
	}

	bool IsOpaqueStaticPacket(const FRenderPacket& Packet) {
		return Packet.Material && Packet.Material->BlendState == EBlendState::Opaque && Packet.Material->bUseViewModeShader && !IsSkeletalProxy(Packet.Proxy) && IsStaticInstanceGeometry(Packet.Geometry);
	}

	bool IsOpaqueStaticGroup(const FMeshDrawGroup& Group) {
		return !Group.bUseSkinning && Group.Material && Group.Material->bUseViewModeShader && Group.Material->BlendState == EBlendState::Opaque && IsStaticInstanceGeometry(Group.Geometry) && std::none_of(Group.Items.begin(), Group.Items.end(), [](const FMeshDrawItem& Item) { return IsSkeletalProxy(Item.Proxy); });
	}

	FMeshDrawGroup& FindStaticDrawGroup(TArray<FMeshDrawGroup>& Groups, FStaticDrawLookup& Lookup, const FGeometryBinding& Geometry, UMaterial* Material) {
		const FStaticDrawKey Key{ Material, Geometry.VertexBuffers[0], Geometry.IndexBuffer, Geometry.StartIndex, Geometry.IndexCount };
		const auto [Iterator, Added]{ Lookup.try_emplace(Key, Groups.Num()) };
		if (Added) {
			FMeshDrawGroup& Group{ Groups.AddDefaulted_GetRef() };
			Group.Geometry = Geometry;
			Group.Material = Material;
		}
		return Groups[Iterator->second];
	}

	bool UploadStaticInstances(const TArray<FMeshDrawGroup>& Groups, TUniquePtr<FVertexBuffer>& Buffer) {
		uint32 InstanceCount{};
		for (const FMeshDrawGroup& Group : Groups) {
			InstanceCount += static_cast<uint32>(Group.Instances.Num());
		}
		if (InstanceCount == 0) {
			return false;
		}

		const uint32 DataSize{ InstanceCount * static_cast<uint32>(sizeof(FObjectInstance)) };
		if (!Buffer || Buffer->GetBufferSize() < DataSize) {
			Buffer = RenderCommand::CreateDynamicVertexBuffer(DataSize, sizeof(FObjectInstance));
		}
		if (!Buffer || !Buffer->GetBuffer()) {
			return false;
		}

		FObjectInstance* Destination{ static_cast<FObjectInstance*>(RenderCommand::MapWriteDiscard(Buffer.get())) };
		if (!Destination) {
			return false;
		}
		for (const FMeshDrawGroup& Group : Groups) {
			std::memcpy(Destination, Group.Instances.GetData(), Group.Instances.Num() * sizeof(FObjectInstance));
			Destination += Group.Instances.Num();
		}
		RenderCommand::Unmap(Buffer.get());
		return true;
	}
}

bool FRenderer::Init()
{
	bUsePerObjectSlots = RenderCommand::SupportsConstantBufferOffsets();
	PerObjectCB = RenderCommand::CreateConstantBuffer(sizeof(FPerObjectConstants));
	ViewCB = RenderCommand::CreateConstantBuffer(sizeof(FViewConstants));
	ShadowViewCB = RenderCommand::CreateConstantBuffer(sizeof(FMatrix));
	LightingCB = RenderCommand::CreateConstantBuffer(sizeof(FLightingConstants));

	FLightingConstants LightingConstant{};
	RenderCommand::UpdateBufferData(LightingCB.get(), &LightingConstant);

	FShaderProgramDesc ShadowStaticSPDesc;
	ShadowStaticSPDesc.Path = "Resources/Shader/ShadowMap.hlsl";
	ShadowStaticSPDesc.Defines = {
		{ "USE_SKINNING", "0" }
	};
	ShadowStaticSPDesc.InputLayout = {
		{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, static_cast<UINT>(offsetof(FVertexPNCT, Position)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, static_cast<UINT>(offsetof(FVertexPNCT, Normal)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(FVertexPNCT, Tangent)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(FVertexPNCT, Color)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, static_cast<UINT>(offsetof(FVertexPNCT, UV)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	ShadowStaticPipeline.Shader = FRenderResourceManager::RegisterProgram("Shadow_Static", ShadowStaticSPDesc);

	FShaderProgramDesc ShadowSkinningSPDesc;
	ShadowSkinningSPDesc.Path = "Resources/Shader/ShadowMap.hlsl";
	ShadowSkinningSPDesc.Defines = {
		{ "USE_SKINNING", "1" }
	};
	ShadowSkinningSPDesc.InputLayout = {
		{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, static_cast<UINT>(offsetof(FVertexPNCT, Position)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, static_cast<UINT>(offsetof(FVertexPNCT, Normal)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(FVertexPNCT, Tangent)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(FVertexPNCT, Color)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, static_cast<UINT>(offsetof(FVertexPNCT, UV)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT,  1, static_cast<UINT>(offsetof(FModelVertexInfluences, BoneIndices)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, static_cast<UINT>(offsetof(FModelVertexInfluences, Weights)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONECOUNT",	  0, DXGI_FORMAT_R32_SINT,			 1, static_cast<UINT>(offsetof(FModelVertexInfluences, InfluenceCount)), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	ShadowSkinningPipeline.Shader = FRenderResourceManager::RegisterProgram("Shadow_Skinned", ShadowSkinningSPDesc);

	FShaderProgramDesc SurfaceStaticSPDesc;
	SurfaceStaticSPDesc.Path = "Resources/Shader/SurfaceShader.hlsl";
	SurfaceStaticSPDesc.Defines = {
		{ "USE_SKINNING", "0" }
	};
	SurfaceStaticSPDesc.InputLayout = {
		{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, static_cast<UINT>(offsetof(FVertexPNCT, Position)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, static_cast<UINT>(offsetof(FVertexPNCT, Normal)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(FVertexPNCT, Tangent)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(FVertexPNCT, Color)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, static_cast<UINT>(offsetof(FVertexPNCT, UV)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	SurfaceStaticPipeline.Shader = FRenderResourceManager::RegisterProgram("Surface_Static", SurfaceStaticSPDesc);

	FShaderProgramDesc SurfaceSkinningSPDesc;
	SurfaceSkinningSPDesc.Path = "Resources/Shader/SurfaceShader.hlsl";
	SurfaceSkinningSPDesc.Defines = {
		{ "USE_SKINNING", "1" }
	};
	SurfaceSkinningSPDesc.InputLayout = {
		{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, static_cast<UINT>(offsetof(FVertexPNCT, Position)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, static_cast<UINT>(offsetof(FVertexPNCT, Normal)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",      0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(FVertexPNCT, Tangent)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(offsetof(FVertexPNCT, Color)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, static_cast<UINT>(offsetof(FVertexPNCT, UV)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_SINT,  1, static_cast<UINT>(offsetof(FModelVertexInfluences, BoneIndices)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, static_cast<UINT>(offsetof(FModelVertexInfluences, Weights)), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONECOUNT",	  0, DXGI_FORMAT_R32_SINT,			 1, static_cast<UINT>(offsetof(FModelVertexInfluences, InfluenceCount)), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	SurfaceSkinningPipeline.Shader = FRenderResourceManager::RegisterProgram("Surface_Skinned", SurfaceSkinningSPDesc);

	ShadowStaticSPDesc.Defines.Add({ "USE_INSTANCING", "1" });
	ShadowStaticSPDesc.InputLayout = FRenderResourceManager::GetStaticMeshInstanceLayout();
	mShadowInstancedPipeline.Shader = FRenderResourceManager::RegisterProgram("Shadow_Static_Instanced", ShadowStaticSPDesc);
	SurfaceStaticSPDesc.Defines.Add({ "USE_INSTANCING", "1" });
	SurfaceStaticSPDesc.InputLayout = FRenderResourceManager::GetStaticMeshInstanceLayout();
	mSurfaceInstancedPipeline.Shader = FRenderResourceManager::RegisterProgram("Surface_Static_Instanced", SurfaceStaticSPDesc);

	TileLightCulling.Init();
	GPUOcclusion.Init();   // 실패해도 오클루전만 못 쓸 뿐 렌더링은 된다

	return true;
}

// 필요한 칸 수가 용량을 넘을 때만 두 배씩 키워 재할당을 드물게 한다.
void FRenderer::EnsurePerObjectSlotCapacity(uint32 SlotCount)
{
	if (SlotCount <= PerObjectSlotCapacity)
		return;

	uint32 NewCapacity = std::max(PerObjectSlotCapacity * 2, MinPerObjectSlots);
	while (NewCapacity < SlotCount)
		NewCapacity *= 2;

	PerObjectSlotCB = RenderCommand::CreateConstantBuffer(NewCapacity * ObjectSlotBytes);
	if (!PerObjectSlotCB || !PerObjectSlotCB->GetBuffer())
	{
		HTR_LOG(Warning, "Per-object constant buffer ({} slots) creation failed. Falling back to per-draw updates.", NewCapacity);
		PerObjectSlotCB.reset();
		PerObjectSlotCapacity = 0;
		return;
	}
	PerObjectSlotCapacity = NewCapacity;
}

// 원래 패킷 순서로 World·Normal 행렬을 올린다. 정렬 목록은 원래 패킷 번호의 칸을 바인딩한다.
void FRenderer::UploadPerObjectConstants()
{
	SCOPE_CYCLE_COUNTER(STAT_UploadPerObjectCB);

	const uint32 Count = static_cast<uint32>(RenderPackets.size());
	if (!bUsePerObjectSlots || Count == 0)
		return;

	EnsurePerObjectSlotCapacity(Count);
	if (!PerObjectSlotCB)
	{
		bUsePerObjectSlots = false;
		return;
	}

	uint8* Dest = static_cast<uint8*>(RenderCommand::MapWriteDiscard(PerObjectSlotCB.get()));
	if (!Dest)
	{
		bUsePerObjectSlots = false;
		return;
	}

	// 매핑된 메모리는 write-combined라 순차 쓰기만 하고 읽지 않는다.
	for (uint32 Index = 0; Index < Count; ++Index)
	{
		FRenderPacket& P = RenderPackets[Index];
		const uint32 ObjectMask = P.Proxy ? P.Proxy->GetObjectMask() : 0u;
		WriteObjectSlot(Dest, Index, GetPacketWorld(P), GetPacketNormalMatrix(P), ObjectMask);
		P.ObjectConstantSlot = Index;
	}

	RenderCommand::Unmap(PerObjectSlotCB.get());
}

// 불투명 우선·반투명 거리순으로 정렬해 View 행렬과 Section 범위로 그린다.
void FRenderer::RenderAll(FRenderQueue& InQueue, const FMatrix& ViewProjection, const FVector& CameraPosition)
{
	RenderQueueSorting(InQueue, ViewProjection, CameraPosition);
	RenderOpaque();
	RenderTranslucent();
}

void FRenderer::RenderSurface()
{
	SCOPE_CYCLE_COUNTER(STAT_DrawRenderPackets);

	RenderCommand::BindConstantBuffer(0, ViewCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
	DrawOpaqueStaticGroups(true);

	FPipelineState* LastPipelinePtr = nullptr;
	for (const FMeshDrawGroup* Group : MeshDrawGroups)
	{
		if (mOpaqueInstancesReady && IsOpaqueStaticGroup(*Group)) {
			continue;
		}
		RenderCommand::BindVertexBuffers(Group->Geometry.VertexBuffers, Group->Geometry.VertexBufferCount);
		RenderCommand::BindIndexBuffer(Group->Geometry.IndexBuffer);

		for (int32 ItemIndex = 0; ItemIndex < Group->Items.Num(); ++ItemIndex)
		{
			const FMeshDrawItem& Item = Group->Items[ItemIndex];
			const FObjectInstance& Instance = Group->Instances[ItemIndex];
			const bool bUseSkinning = Group->bUseSkinning;
			FPipelineState* Pipeline = bUseSkinning ? &SurfaceSkinningPipeline : &SurfaceStaticPipeline;

			if (LastPipelinePtr != Pipeline)
			{
				RenderCommand::BindPipelineState(*Pipeline);
				LastPipelinePtr = Pipeline;
			}

			if (bUsePerObjectSlots && Item.ObjectConstantSlot != InvalidObjectConstantSlot)
			{
				RenderCommand::BindConstantBufferRange(1, PerObjectSlotCB.get(), Item.ObjectConstantSlot * PerObjectSlotConstants, PerObjectSlotConstants, EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
			}
			else
			{
				RenderCommand::BindConstantBuffer(1, PerObjectCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
				UpdatePerObjectConstants(Instance.Model,
					Instance.NormalMatrix,
					Item.Proxy ? Item.Proxy->GetObjectMask() : 0u);
			}

			if (bUseSkinning)
			{
				RenderCommand::BindShaderResource(0, Item.Proxy->GetBoneMatricesSRV(), EShaderBindFlagBits::Vertex);
			}

			RenderCommand::DrawIndexed(Group->Geometry.IndexCount, Group->Geometry.StartIndex);
		}
	}

	for (uint32 Index = 0; Index < FirstTranslucentIndex; ++Index)
	{
		const FRenderPacket& Packet = RenderPackets[Index];
		if (mOpaqueInstancesReady && IsOpaqueStaticPacket(Packet)) {
			continue;
		}
		const bool bUseSkinning = Packet.Proxy && Packet.Proxy->IsUsingSkinning();

		FPipelineState* Pipeline = bUseSkinning ? &SurfaceSkinningPipeline : &SurfaceStaticPipeline;

		if (LastPipelinePtr != Pipeline)
		{
			RenderCommand::BindPipelineState(*Pipeline);
			LastPipelinePtr = Pipeline;
		}

		RenderCommand::BindVertexBuffers(Packet.Geometry.VertexBuffers, Packet.Geometry.VertexBufferCount);
		RenderCommand::BindIndexBuffer(Packet.Geometry.IndexBuffer);

		if (bUsePerObjectSlots && Packet.ObjectConstantSlot != InvalidObjectConstantSlot)
		{
			RenderCommand::BindConstantBufferRange(1, PerObjectSlotCB.get(), Packet.ObjectConstantSlot * PerObjectSlotConstants, PerObjectSlotConstants, EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
		}
		else
		{
			RenderCommand::BindConstantBuffer(1, PerObjectCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
			UpdatePerObjectConstants(Packet);
		}

		if (bUseSkinning)
		{
			RenderCommand::BindShaderResource(0, Packet.Proxy->GetBoneMatricesSRV(), EShaderBindFlagBits::Vertex);
		}

		RenderCommand::DrawIndexed(Packet.Geometry.IndexCount, Packet.Geometry.StartIndex);
	}

	RenderCommand::UnbindShaderResource(0, EShaderBindFlagBits::Vertex);

	LastVertexBufferCount = 0;
	LastIndexBuffer = nullptr;
	LastMaterial = nullptr;
}

void FRenderer::RenderOpaque()
{
	TileLightCulling.BindForShading(LocalLight);
	DrawOpaqueStaticGroups(false);
	DrawMeshGroups();
	DrawPackets(0, FirstTranslucentIndex);
	TileLightCulling.UnbindForShading();
}

void FRenderer::RenderTranslucent()
{
	TileLightCulling.BindForShading(LocalLight);
	DrawTranslucentMeshGroups();
	TileLightCulling.UnbindForShading();

	RenderPackets.Reset();
	FirstTranslucentIndex = 0;
	MeshDrawGroups.clear();   // 묶음 메모리는 World 것이므로 이번 프레임이 끝나면 놓는다
	TranslucentMeshDrawGroups.Reset();
}

// 메시 묶음을 정렬 키 순서로 그린다. 바인딩은 묶음마다 한 번, 항목마다는 칸 바인딩과 드로우만 한다.
void FRenderer::DrawMeshGroups()
{
	if (MeshDrawGroups.empty())
		return;

	SCOPE_CYCLE_COUNTER(STAT_DrawRenderPackets);
	// 픽셀 단위 조명이 카메라 위치를 읽으므로 PS에도 바인딩한다.
	RenderCommand::BindConstantBuffer(0, ViewCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
	RenderCommand::BindConstantBuffer(3, LightingCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);

	UMaterial* BoundMaterial = nullptr;
	int8 LastUseSkinning = -1;

	for (const FMeshDrawGroup* Group : MeshDrawGroups)
	{
		if (mOpaqueInstancesReady && IsOpaqueStaticGroup(*Group)) {
			continue;
		}
		RenderCommand::BindVertexBuffers(Group->Geometry.VertexBuffers, Group->Geometry.VertexBufferCount);
		RenderCommand::BindIndexBuffer(Group->Geometry.IndexBuffer);
		for (int32 ItemIndex = 0; ItemIndex < Group->Items.Num(); ++ItemIndex)
		{
			const FMeshDrawItem& Item = Group->Items[ItemIndex];
			const FObjectInstance& Instance = Group->Instances[ItemIndex];
			const bool bUseSkinning = Group->bUseSkinning;
			const int8 CurrentSkinning = bUseSkinning ? 1 : 0;
			const bool bMaterialChanged = (Group->Material != BoundMaterial);
			const bool bSkinningChanged = CurrentSkinning != LastUseSkinning;

			if (bMaterialChanged || bSkinningChanged)
			{
				BoundMaterial = Group->Material;
				LastUseSkinning = CurrentSkinning;
				BindMaterial(BoundMaterial, bUseSkinning);
				if (bMaterialChanged)
				{
					UpdateMaterialParams(BoundMaterial);
				}
			}

			if (bUsePerObjectSlots && Item.ObjectConstantSlot != InvalidObjectConstantSlot)
			{
				RenderCommand::BindConstantBufferRange(2, PerObjectSlotCB.get(), Item.ObjectConstantSlot * PerObjectSlotConstants, PerObjectSlotConstants, EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
			}
			else
			{
				RenderCommand::BindConstantBuffer(2, PerObjectCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
				UpdatePerObjectConstants(Instance.Model,
					Instance.NormalMatrix,
					Item.Proxy ? Item.Proxy->GetObjectMask() : 0u);
			}

			if (Item.Proxy)
			{
				BindSurfaceSkinningResources(Item.Proxy);
			}

			RenderCommand::DrawIndexed(Group->Geometry.IndexCount, Group->Geometry.StartIndex);
		}
	}

	// 뒤따르는 DrawPackets가 처음부터 다시 바인딩하도록 기록을 비운다.
	LastVertexBufferCount = 0;
	LastIndexBuffer = nullptr;
	LastMaterial = nullptr;
}

void FRenderer::DrawTranslucentMeshGroups() {
	SCOPE_CYCLE_COUNTER(STAT_DrawRenderPackets);

	RenderCommand::BindConstantBuffer(0, ViewCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
	RenderCommand::BindConstantBuffer(3, LightingCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);

	for (const FMeshDrawGroup& Group : TranslucentMeshDrawGroups) {
		RenderCommand::BindVertexBuffers(Group.Geometry.VertexBuffers, Group.Geometry.VertexBufferCount);
		RenderCommand::BindIndexBuffer(Group.Geometry.IndexBuffer);

		const bool UseSkinning{ Group.bUseSkinning };
		BindMaterial(Group.Material, UseSkinning);

		if (Group.Material->bUseViewModeShader) {
			RenderCommand::BindConstantBuffer(2, PerObjectCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);

			for (int32 Index{}; Index < Group.Instances.Num(); ++Index) {
				const FObjectInstance& Instance{ Group.Instances[Index] };
				const FPrimitiveSceneProxy* Proxy{ Group.Items[Index].Proxy };
				UpdateMaterialParams(Group.Material, Instance.Color);
				UpdatePerObjectConstants(Instance.Model, Instance.NormalMatrix, Proxy ? Proxy->GetObjectMask() : 0u);
				if (Proxy) {
					BindSurfaceSkinningResources(Proxy);
				}
				RenderCommand::DrawIndexed(Group.Geometry.IndexCount, Group.Geometry.StartIndex);
			}

			// 조명 셰이더 머티리얼은 인스턴싱 없이 항목마다 개별 드로우
			if (!Group.Items.IsEmpty() && IsParticleProxy(Group.Items[0].Proxy)) {
				FParticleStats::GetPending().DrawCalls += Group.Items.Num();
			}
			continue;
		}

		const uint32 InstanceCount{ static_cast<uint32>(Group.Instances.Num()) };
		const uint32 InstanceDataSize{ InstanceCount * sizeof(FObjectInstance) };
		if (!ObjectInstanceVB || ObjectInstanceVB->GetBufferSize() < InstanceDataSize) {
			ObjectInstanceVB = RenderCommand::CreateDynamicVertexBuffer(InstanceDataSize, sizeof(FObjectInstance));
		}

		FObjectInstance* MappedInstances{ static_cast<FObjectInstance*>(RenderCommand::MapWriteDiscard(ObjectInstanceVB.get())) };
		std::memcpy(MappedInstances, Group.Instances.GetData(), InstanceDataSize);
		RenderCommand::Unmap(ObjectInstanceVB.get());

		RenderCommand::BindVertexBuffer(Group.Geometry.VertexBufferCount, ObjectInstanceVB.get());
		UpdateMaterialParams(Group.Material);

		if (UseSkinning) {
			// NOTE: 스키닝 메시를 그릴때는 본 메트릭스가 다를 수 있기 때문에 개별 드로우로 진행한다.
			for (int32 Index{}; Index < Group.Items.Num(); ++Index) {
				const FMeshDrawItem& Item{ Group.Items[Index] };

				BindSurfaceSkinningResources(Item.Proxy);
				RenderCommand::DrawIndexedInstanced(Group.Geometry.IndexCount, 1, Group.Geometry.StartIndex, 0, Index);
			}
		} else {
			RenderCommand::DrawIndexedInstanced(Group.Geometry.IndexCount, InstanceCount, Group.Geometry.StartIndex);

			if (!Group.Items.IsEmpty() && IsParticleProxy(Group.Items[0].Proxy))
			{
				FParticleFrameStats& Stats = FParticleStats::GetPending();
				Stats.DrawCalls++;
				Stats.InstancedParticles += InstanceCount;
			}
		}
	}
}

void FRenderer::RenderQueueSorting(FRenderQueue& InQueue, const FMatrix& ViewProjection, const FVector& CameraPosition)
{
	FViewConstants ViewConstants;
	ViewConstants.ViewProjection = ViewProjection;
	ViewConstants.CameraPosition = CameraPosition;

	RenderCommand::UpdateBufferData(ViewCB.get(), &ViewConstants);
	{
		SCOPE_CYCLE_COUNTER(STAT_RenderQueueSorting);

		std::swap(RenderPackets, InQueue);
		InQueue.Reset();

		std::sort(RenderPackets.begin(), RenderPackets.end(), [](const FRenderPacket& A, const FRenderPacket& B) {
			if (A.Material->BlendState != B.Material->BlendState)
			{
				return A.Material->BlendState < B.Material->BlendState;
			}

			if (A.Material->BlendState != EBlendState::Opaque)
			{
				return A.CameraDistanceSquared > B.CameraDistanceSquared;
			}

			if (A.Material->SortID != B.Material->SortID)
			{
				return A.Material->SortID < B.Material->SortID;
			}

			if (A.Geometry.VertexBufferCount != B.Geometry.VertexBufferCount)
			{
				return A.Geometry.VertexBufferCount < B.Geometry.VertexBufferCount;
			}

			for (int32 i = 0; i < A.Geometry.VertexBufferCount; i++)
			{
				if (A.Geometry.VertexBuffers[i] != B.Geometry.VertexBuffers[i])
				{
					return A.Geometry.VertexBuffers[i] < B.Geometry.VertexBuffers[i];
				}
			}

			return A.Geometry.IndexBuffer < B.Geometry.IndexBuffer;
		});

		FirstTranslucentIndex = 0;
		for (int32 i = 0; i < RenderPackets.Num(); i++)
		{
			if (RenderPackets[i].Material->BlendState == EBlendState::AlphaBlend)
			{
				break;
			}

			++FirstTranslucentIndex;
		}

		// 메시 묶음: 빈 묶음을 빼고 키 순으로 정렬한다. 조각마다 같은 키 묶음이 있으므로 정렬하면 서로 붙는다.
		std::erase_if(MeshDrawGroups, [](const FMeshDrawGroup* Group) { return Group->Items.IsEmpty(); });
		std::sort(MeshDrawGroups.begin(), MeshDrawGroups.end(), [](const FMeshDrawGroup* A, const FMeshDrawGroup* B) {
			if (A->Material->SortID != B->Material->SortID)
			{
				return A->Material->SortID < B->Material->SortID;
			}

			if (A->Geometry.VertexBufferCount != B->Geometry.VertexBufferCount)
			{
				return A->Geometry.VertexBufferCount < B->Geometry.VertexBufferCount;
			}

			for (int32 i = 0; i < A->Geometry.VertexBufferCount; ++i)
			{
				if (A->Geometry.VertexBuffers[i] != B->Geometry.VertexBuffers[i])
				{
					return A->Geometry.VertexBuffers[i] < B->Geometry.VertexBuffers[i];
				}
			}

			return A->Geometry.IndexBuffer < B->Geometry.IndexBuffer;
		});

		BuildTranslucentGroups();
		BuildOpaqueStaticGroups();
	}

	// Gather uploads group and packet slots together. Other queues need an upload here.
	if (!bObjectConstantsPrepared)
	{
		UploadPerObjectConstants();
	}

	bObjectConstantsPrepared = false;
}

void FRenderer::BuildOpaqueStaticGroups() {
	mOpaqueStaticGroups.Reset();
	FStaticDrawLookup Lookup{};
	for (const FMeshDrawGroup* Source : MeshDrawGroups) {
		if (!IsOpaqueStaticGroup(*Source)) {
			continue;
		}
		FMeshDrawGroup& Group{ FindStaticDrawGroup(mOpaqueStaticGroups, Lookup, Source->Geometry, Source->Material) };
		for (int32 Index{}; Index < Source->Instances.Num(); ++Index) {
			FObjectInstance Instance{ Source->Instances[Index] };
			const FMeshDrawItem& Item{ Source->Items[Index] };
			Instance.ObjectMask = Item.Proxy ? Item.Proxy->GetObjectMask() : Instance.ObjectMask;
			Group.Instances.Add(Instance);
		}
	}
	for (uint32 Index{}; Index < FirstTranslucentIndex; ++Index) {
		const FRenderPacket& Packet{ RenderPackets[Index] };
		if (!IsOpaqueStaticPacket(Packet)) {
			continue;
		}
		FMeshDrawGroup& Group{ FindStaticDrawGroup(mOpaqueStaticGroups, Lookup, Packet.Geometry, Packet.Material) };
		Group.Instances.Add(FObjectInstance{ GetPacketWorld(Packet), GetPacketNormalMatrix(Packet), Packet.ColorMultiplier, Packet.Proxy ? Packet.Proxy->GetObjectMask() : 0u });
	}
	mOpaqueInstancesReady = UploadStaticInstances(mOpaqueStaticGroups, mOpaqueInstanceVB);
}

void FRenderer::DrawOpaqueStaticGroups(bool SurfacePass) {
	if (!mOpaqueInstancesReady) {
		return;
	}
	RenderCommand::BindConstantBuffer(0, ViewCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
	if (SurfacePass) {
		RenderCommand::BindPipelineState(mSurfaceInstancedPipeline);
	} else {
		RenderCommand::BindConstantBuffer(3, LightingCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
		if (!mInstancedViewModeShader) {
			SetViewMode(FViewRenderMode{});
		}
	}

	uint32 FirstInstance{};
	for (const FMeshDrawGroup& Group : mOpaqueStaticGroups) {
		if (!SurfacePass) {
			BindMaterial(Group.Material, false);
			RenderCommand::BindShaderProgram(mInstancedViewModeShader);
			UpdateMaterialParams(Group.Material);
		}
		RenderCommand::BindVertexBuffers(Group.Geometry.VertexBuffers, Group.Geometry.VertexBufferCount);
		RenderCommand::BindIndexBuffer(Group.Geometry.IndexBuffer);
		RenderCommand::BindVertexBuffer(1, mOpaqueInstanceVB.get());
		const uint32 InstanceCount{ static_cast<uint32>(Group.Instances.Num()) };
		RenderCommand::DrawIndexedInstanced(Group.Geometry.IndexCount, InstanceCount, Group.Geometry.StartIndex, 0, FirstInstance);
		FirstInstance += InstanceCount;
	}
	RenderCommand::BindVertexBuffer(1, nullptr);
	LastVertexBufferCount = 0;
	LastIndexBuffer = nullptr;
	LastMaterial = nullptr;
}

void FRenderer::BuildShadowStaticGroups(const TArray<FPrimitiveSceneProxy*>& Casters) {
	mShadowStaticGroups.Reset();
	FStaticDrawLookup Lookup{};
	for (FPrimitiveSceneProxy* Proxy : Casters) {
		if (!Proxy || !Proxy->IsVisible() || !Proxy->IsCastShadow() || IsSkeletalProxy(Proxy)) {
			continue;
		}
		const FMeshRenderState* State{ Proxy->GetRenderState() };
		if (State && State->LODCount > 0 && State->LODs[0].NumSections > 0) {
			const FCachedMeshLOD& LOD{ State->LODs[0] };
			for (uint32 Index{}; Index < LOD.NumSections; ++Index) {
				const FCachedMeshSection& Section{ Proxy->GetSection(LOD.FirstSection + Index) };
				if (Section.Material && Section.Material->BlendState != EBlendState::Opaque) {
					continue;
				}
				FGeometryBinding Geometry{};
				Geometry.SetVertexBuffers(LOD.VertexBuffers, LOD.VertexBufferCount);
				Geometry.IndexBuffer = LOD.IndexBuffer;
				Geometry.StartIndex = Section.StartIndex;
				Geometry.IndexCount = Section.IndexCount;
				if (!IsStaticInstanceGeometry(Geometry)) {
					continue;
				}
				FMeshDrawGroup& Group{ FindStaticDrawGroup(mShadowStaticGroups, Lookup, Geometry, Section.Material) };
				Group.Instances.Add(FObjectInstance{ Proxy->GetLocalToWorld(), Proxy->GetNormalMatrix(), FLinearColor::White, Proxy->GetObjectMask() });
			}
		} else if (Cast<UParticleSystemComponent>(Proxy->GetComponent())) {
			FRenderQueue Queue{};
			const FViewBasis View{};
			Proxy->SubmitToRenderQueue(Queue, View);
			for (const FRenderPacket& Packet : Queue) {
				if (!Packet.Material || Packet.Material->BlendState != EBlendState::Opaque || !IsStaticInstanceGeometry(Packet.Geometry)) {
					continue;
				}
				FMeshDrawGroup& Group{ FindStaticDrawGroup(mShadowStaticGroups, Lookup, Packet.Geometry, Packet.Material) };
				Group.Instances.Add(FObjectInstance{ GetPacketWorld(Packet), GetPacketNormalMatrix(Packet), Packet.ColorMultiplier, Proxy->GetObjectMask() });
			}
		}
	}
	mShadowInstancesReady = UploadStaticInstances(mShadowStaticGroups, mShadowInstanceVB);
}

void FRenderer::DrawShadowStaticGroups(FShadowStats& ShadowStats) {
	if (!mShadowInstancesReady) {
		return;
	}
	RenderCommand::BindPipelineState(mShadowInstancedPipeline);
	uint32 FirstInstance{};
	for (const FMeshDrawGroup& Group : mShadowStaticGroups) {
		RenderCommand::BindVertexBuffers(Group.Geometry.VertexBuffers, Group.Geometry.VertexBufferCount);
		RenderCommand::BindIndexBuffer(Group.Geometry.IndexBuffer);
		RenderCommand::BindVertexBuffer(1, mShadowInstanceVB.get());
		const uint32 InstanceCount{ static_cast<uint32>(Group.Instances.Num()) };
		RenderCommand::DrawIndexedInstanced(Group.Geometry.IndexCount, InstanceCount, Group.Geometry.StartIndex, 0, FirstInstance);
		FirstInstance += InstanceCount;
		++ShadowStats.NumShadowDrawCalls;
	}
	RenderCommand::BindVertexBuffer(1, nullptr);
}

void FRenderer::BuildTranslucentGroups()
{
	TranslucentMeshDrawGroups.Reset();

	FMeshDrawGroup* CurrentMeshGroup = nullptr;
	for (int32 i = FirstTranslucentIndex; i < RenderPackets.Num(); i++)
	{
		const FRenderPacket& RenderPacket = RenderPackets[i];
		const bool bUseSkinning = RenderPacket.Proxy && RenderPacket.Proxy->IsUsingSkinning();

		if (!CurrentMeshGroup || CurrentMeshGroup->Material != RenderPacket.Material || CurrentMeshGroup->Geometry != RenderPacket.Geometry || CurrentMeshGroup->bUseSkinning != bUseSkinning)
		{
			CurrentMeshGroup = &TranslucentMeshDrawGroups.AddDefaulted_GetRef();
			CurrentMeshGroup->Material = RenderPacket.Material;
			CurrentMeshGroup->Geometry = RenderPacket.Geometry;
			CurrentMeshGroup->bUseSkinning = bUseSkinning;
		}

		FMeshDrawItem& DrawItem = CurrentMeshGroup->Items.AddDefaulted_GetRef();
		DrawItem.Proxy = RenderPacket.Proxy;
		DrawItem.ObjectConstantSlot = RenderPacket.ObjectConstantSlot;
		DrawItem.bOccludedByGpu = RenderPacket.bOccludedByGpu;

		FObjectInstance& Instance = CurrentMeshGroup->Instances.AddDefaulted_GetRef();
		Instance.Model = GetPacketWorld(RenderPacket);
		Instance.NormalMatrix = GetPacketNormalMatrix(RenderPacket);
		Instance.Color = RenderPacket.ColorMultiplier;
		Instance.ObjectMask = RenderPacket.Proxy ? RenderPacket.Proxy->GetObjectMask() : 0u;
	}
}

// 정렬된 패킷 중 [Begin, End) 범위를 View 행렬과 Section 범위로 그린다.
void FRenderer::DrawPackets(uint32 Begin, uint32 End)
{
	SCOPE_CYCLE_COUNTER(STAT_DrawRenderPackets);

	LastVertexBufferCount = 0;
	LastIndexBuffer = nullptr;
	LastMaterial = nullptr;
	int8 LastUseSkinning = -1;
	FLinearColor LastColor{ FLinearColor::White };

	RenderCommand::BindConstantBuffer(0, ViewCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
	RenderCommand::BindConstantBuffer(3, LightingCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
	// k = 정렬된 위치
	for (uint32 k = Begin; k < End; ++k)
	{
		const FRenderPacket& RenderPacket = RenderPackets[k];
		if (mOpaqueInstancesReady && IsOpaqueStaticPacket(RenderPacket)) {
			continue;
		}

		// 불투명 파티클은 패킷마다 개별 드로우
		if (IsParticleProxy(RenderPacket.Proxy))
		{
			FParticleStats::GetPending().DrawCalls++;
		}

		if (!RenderPacket.Geometry.HasSameBuffers(LastVertexBuffers, LastVertexBufferCount, LastIndexBuffer))
		{
			RenderCommand::BindVertexBuffers(RenderPacket.Geometry.VertexBuffers, RenderPacket.Geometry.VertexBufferCount);
			RenderCommand::BindIndexBuffer(RenderPacket.Geometry.IndexBuffer);

			std::memcpy(LastVertexBuffers, RenderPacket.Geometry.VertexBuffers, sizeof(FVertexBuffer*) * RenderPacket.Geometry.VertexBufferCount);
			LastVertexBufferCount = RenderPacket.Geometry.VertexBufferCount;
			LastIndexBuffer = RenderPacket.Geometry.IndexBuffer;
		}
		const bool bUseSkinning = RenderPacket.Proxy ? RenderPacket.Proxy->IsUsingSkinning() : false;
		const int8 CurrentSkinning = bUseSkinning ? 1 : 0;
		if (RenderPacket.Material != LastMaterial || CurrentSkinning != LastUseSkinning)
		{
			BindMaterial(RenderPacket.Material, bUseSkinning);
		}

		const FLinearColor& Color{ RenderPacket.ColorMultiplier };
		if (RenderPacket.Material != LastMaterial || Color.R != LastColor.R || Color.G != LastColor.G || Color.B != LastColor.B || Color.A != LastColor.A) {
			UpdateMaterialParams(RenderPacket.Material, Color);
			LastColor = Color;
		}

		if (bUsePerObjectSlots && RenderPacket.ObjectConstantSlot != InvalidObjectConstantSlot)
		{
			RenderCommand::BindConstantBufferRange(2, PerObjectSlotCB.get(), RenderPacket.ObjectConstantSlot * PerObjectSlotConstants, PerObjectSlotConstants, EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
		}
		else
		{
			RenderCommand::BindConstantBuffer(2, PerObjectCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
			UpdatePerObjectConstants(RenderPacket);
		}

		if (RenderPacket.Proxy)
		{
			BindSurfaceSkinningResources(RenderPacket.Proxy);
		}

		RenderCommand::DrawIndexed(RenderPacket.Geometry.IndexCount, RenderPacket.Geometry.StartIndex);

		LastMaterial = RenderPacket.Material;
		LastUseSkinning = CurrentSkinning;
	}
}

FOcclusionMeasureResult FRenderer::MeasureOpaqueOcclusion()
{
	FOcclusionMeasureResult Result;

	// 불투명 드로우를 두 경로(스태틱 묶음, 일반 패킷)에서 한 목록으로 모은다. 그린 순서와 같게 묶음 먼저.
	struct FMeasureDraw
	{
		const FGeometryBinding* Geometry;
		UMaterial* Material;
		uint32 ObjectConstantSlot;
		uint32 StartIndex;
		uint32 IndexCount;
		const FMatrix* World;       // 칸이 없을 때만 쓴다
		const void* ObjectKey;      // 같은 물체의 섹션을 한 물체로 센다
		bool bOccludedByGpu;
		const FRenderPacket* Packet;
		bool bUseSkinning;
		const FPrimitiveSceneProxy* Proxy;
	};

	std::vector<FMeasureDraw> Draws;
	for (const FMeshDrawGroup* Group : MeshDrawGroups)
	{
		for (int32 ItemIndex = 0; ItemIndex < Group->Items.Num(); ++ItemIndex)
		{
			const FMeshDrawItem& Item = Group->Items[ItemIndex];
			const FObjectInstance& Instance = Group->Instances[ItemIndex];
			if (!Group->Geometry.VertexBufferCount || !Group->Geometry.IndexBuffer || !Group->Material || !Item.Proxy || !Group->Geometry.IndexCount)
				continue;
			Draws.push_back({ &Group->Geometry, Group->Material, Item.ObjectConstantSlot, Group->Geometry.StartIndex, Group->Geometry.IndexCount, &Instance.Model, Item.Proxy, Item.bOccludedByGpu != 0, nullptr, Group->bUseSkinning, Item.Proxy });
		}
	}

	for (uint32 k = 0; k < FirstTranslucentIndex; ++k)
	{
		const FRenderPacket& Packet = RenderPackets[k];
		if (!Packet.Geometry.VertexBufferCount || !Packet.Geometry.IndexBuffer || !Packet.Material || !Packet.Geometry.IndexCount)
			continue;
		// 프록시가 없는 패킷(빌보드 등)은 패킷 자체를 한 물체로 센다.
		const void* Key = Packet.Proxy ? static_cast<const void*>(Packet.Proxy) : static_cast<const void*>(&Packet);
		Draws.push_back({ &Packet.Geometry, Packet.Material, Packet.ObjectConstantSlot, Packet.Geometry.StartIndex, Packet.Geometry.IndexCount, &GetPacketWorld(Packet), Key, Packet.bOccludedByGpu, &Packet, Packet.Proxy && Packet.Proxy->IsUsingSkinning(), Packet.Proxy });
	}

	const uint32 Count = static_cast<uint32>(Draws.size());
	if (Count == 0)
		return Result;

	const auto StartTime = std::chrono::high_resolution_clock::now();

	ID3D11Device* Device = RenderCommand::GetDevice();
	ID3D11DeviceContext* Context = RenderCommand::GetContext();

	// 같은 셰이더·같은 행렬로 다시 그리면 깊이가 비트 단위로 같으므로,
	// LESS_EQUAL이면 최종 깊이 버퍼에서 이 물체가 이긴 픽셀만 통과한다.
	if (!DepthLessEqualReadOnly)
	{
		D3D11_DEPTH_STENCIL_DESC Desc{};
		Desc.DepthEnable = TRUE;
		Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		Desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		if (FAILED(Device->CreateDepthStencilState(&Desc, DepthLessEqualReadOnly.GetAddressOf())))
			return Result;
	}

	while (OcclusionQueries.Num() < Count)
	{
		const D3D11_QUERY_DESC QueryDesc{ D3D11_QUERY_OCCLUSION, 0 };
		ComPtr<ID3D11Query> Query;
		if (FAILED(Device->CreateQuery(&QueryDesc, Query.GetAddressOf())))
			return Result;
		OcclusionQueries.Add(std::move(Query));
	}

	RenderCommand::BindConstantBuffer(0, ViewCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
	RenderCommand::BindConstantBuffer(3, LightingCB.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
	if (!bUsePerObjectSlots)
		RenderCommand::BindConstantBuffer(2, PerObjectCB.get(), EShaderBindFlagBits::Vertex);

	const FGeometryBinding* BoundGeometry = nullptr;
	UMaterial* BoundMaterial = nullptr;
	bool bBoundUseSkinning = false;

	// 1) 드로우마다 쿼리를 걸고 다시 그린다. 본 패스와 같은 바인딩(칸 또는 PerObjectCB)을 쓴다.
	for (uint32 Index = 0; Index < Count; ++Index)
	{
		const FMeasureDraw& Draw = Draws[Index];

		const FGeometryBinding& Geometry = *Draw.Geometry;
		if (!BoundGeometry || !Geometry.HasSameBuffers(BoundGeometry->VertexBuffers, BoundGeometry->VertexBufferCount, BoundGeometry->IndexBuffer))
		{
			RenderCommand::BindVertexBuffers(Geometry.VertexBuffers, Geometry.VertexBufferCount);
			RenderCommand::BindIndexBuffer(Geometry.IndexBuffer);
		}

		if (Draw.Material != BoundMaterial || Draw.bUseSkinning != bBoundUseSkinning)
		{
			BindMaterial(Draw.Material, Draw.bUseSkinning);
			// BindMaterial이 바꾼 상태를 측정용으로 덮어쓴다.
			RenderCommand::SetBlendState(EBlendState::NoColorWrite);
			Context->OMSetDepthStencilState(DepthLessEqualReadOnly.Get(), 0);
		}
		if (Draw.Material != BoundMaterial)
			UpdateMaterialParams(Draw.Material);

		if (bUsePerObjectSlots && Draw.ObjectConstantSlot != InvalidObjectConstantSlot)
			RenderCommand::BindConstantBufferRange(2, PerObjectSlotCB.get(),
				Draw.ObjectConstantSlot * PerObjectSlotConstants, PerObjectSlotConstants, EShaderBindFlagBits::Vertex);
		else
		{
			RenderCommand::BindConstantBuffer(2, PerObjectCB.get(), EShaderBindFlagBits::Vertex);
			// 패킷이 없는 스태틱 묶음도 포함하는 측정용 행렬 폴백
			const uint32 ObjectMask = Draw.Proxy ? Draw.Proxy->GetObjectMask() : 0u;
			UpdatePerObjectConstants(*Draw.World, MakeNormalMatrix(*Draw.World), ObjectMask);
		}

		ID3D11Query* Query = OcclusionQueries[Index].Get();
		Context->Begin(Query);
		if (Draw.Proxy)
		{
			BindSurfaceSkinningResources(Draw.Proxy);
		}
		RenderCommand::DrawIndexed(Draw.IndexCount, Draw.StartIndex);
		Context->End(Query);

		BoundGeometry = Draw.Geometry;
		BoundMaterial = Draw.Material;
		bBoundUseSkinning = Draw.bUseSkinning;
	}

	// 2) 결과를 기다려 모은다. 한 물체가 Section 여러 개로 나뉘면 하나라도 보이면 보이는 것으로 친다.
	std::unordered_map<const void*, bool> ObjectVisible;
	ObjectVisible.reserve(Count);
	for (uint32 Index = 0; Index < Count; ++Index)
	{
		const FMeasureDraw& Draw = Draws[Index];

		UINT64 Samples = 0;
		while (Context->GetData(OcclusionQueries[Index].Get(), &Samples, sizeof(Samples), 0) == S_FALSE) {}

		const uint64 Triangles = Draw.IndexCount / 3;
		const bool bVisible = Samples > 0;

		// Cull을 끄고 판정만 한 드로우: 가렸다고 했는데 최종 화면에 픽셀이 남았으면 잘못 가린 것이다.
		if (Draw.bOccludedByGpu)
		{
			++Result.GPUOccludedDraws;
			if (bVisible)
				++Result.FalseCulls;
		}

		++Result.TotalDraws;
		Result.TotalTriangles += Triangles;
		if (bVisible)
		{
			++Result.VisibleDraws;
			Result.VisibleTriangles += Triangles;
		}

		bool& bObjectVisible = ObjectVisible[Draw.ObjectKey];
		bObjectVisible = bObjectVisible || bVisible;
	}

	Result.TotalObjects = static_cast<uint32>(ObjectVisible.size());
	for (const auto& [Key, bVisible] : ObjectVisible)
		Result.VisibleObjects += bVisible ? 1 : 0;

	// 뒤따르는 Grid·반투명 패스를 위해 상태를 되돌린다.
	RenderCommand::SetBlendState(EBlendState::Opaque);
	RenderCommand::SetDepthStencilState(EDepthStencilState::Default);

	Result.ElapsedMs = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - StartTime).count();
	Result.bValid = true;
	return Result;
}

uint8* FRenderer::BeginObjectConstants(uint32 MaxSlots)
{
	bObjectConstantsPrepared = false;
	if (!bUsePerObjectSlots || MaxSlots == 0) return nullptr;
	EnsurePerObjectSlotCapacity(MaxSlots);
	if (!PerObjectSlotCB) { bUsePerObjectSlots = false; return nullptr; }
	return static_cast<uint8*>(RenderCommand::MapWriteDiscard(PerObjectSlotCB.get()));
}

void FRenderer::EndObjectConstants()
{
	if (PerObjectSlotCB)
	{
		RenderCommand::Unmap(PerObjectSlotCB.get());
		bObjectConstantsPrepared = true;
	}
}

void FRenderer::SetViewMode(const FViewRenderMode& Mode)
{
	for (int32 Skinning = 0; Skinning < 2; Skinning++)
	{
		ViewModeShaders[Skinning] = FRenderResourceManager::GetUberLitProgram(Mode, Skinning != 0);
		mTranslucentViewModeShaders[Skinning] = FRenderResourceManager::GetUberLitProgram(Mode, Skinning != 0, false, true);
	}
	mInstancedViewModeShader = FRenderResourceManager::GetUberLitProgram(Mode, false, true);
}

void FRenderer::SetLighting(const FLightingConstants& GlobalLights,
	const TArray<FPointLightInfo>& PointLights, const TArray<FSpotLightInfo>& SpotLights)
{
	// 이전 View의 PS SRV 바인딩을 끊은 뒤 동적 LocalLight 버퍼를 다시 쓴다.
	TileLightCulling.UnbindForShading();
	RenderCommand::UpdateBufferData(LightingCB.get(), &GlobalLights);
	LocalLight.UploadLights(PointLights, SpotLights);
}

// Material마다 Shader/Texture/Sampler/State 꽂기
void FRenderer::BindMaterial(UMaterial* Material, bool UseSkinning) {
	// Shader selection is independent of the common material constant-buffer layout.
	if (Material->bUseViewModeShader)
	{
		if (!ViewModeShaders[0])
		{
			SetViewMode(FViewRenderMode{});
		}

		FShaderProgram* Shader{ Material->BlendState == EBlendState::Opaque ? ViewModeShaders[UseSkinning ? 1 : 0] : mTranslucentViewModeShaders[UseSkinning ? 1 : 0] };
		RenderCommand::BindShaderProgram(Shader);
	}
	else
	{
		RenderCommand::BindShaderProgram(Material->Shader);
	}

	RenderCommand::SetBlendState(Material->BlendState);

	// 반투명은 뒤에 그려지는 Grid·다른 반투명을 가리지 않도록 깊이를 쓰지 않는다.
	const bool bTranslucent = Material->BlendState != EBlendState::Opaque;
	RenderCommand::SetDepthStencilState(bTranslucent && Material->DepthStencilState == EDepthStencilState::Default ? EDepthStencilState::ReadOnly : Material->DepthStencilState);

	for (int i = 0; i < Material->Textures.size(); i++)
	{
		// 같은 파일이 먼저 sRGB로 캐시됐을 수 있어 노멀맵 슬롯은 불러온 방식과 무관하게 원본 값으로 읽는다.
		const bool bNormalMapSlot = i == 1;
		RenderCommand::BindShaderResource(i, Material->Textures[i], EShaderBindFlagBits::Pixel, bNormalMapSlot);
	}

	if (!HasBoundNormalMap(Material))
	{
		RenderCommand::UnbindShaderResource(1, EShaderBindFlagBits::Pixel);
	}

	RenderCommand::BindSamplerState(0, Material->SamplerState, EShaderBindFlagBits::Pixel);
}

// b1 내용 채우고 꽂기
void FRenderer::UpdateMaterialParams(UMaterial* Material, const FLinearColor& ColorMultiplier) {
	if (!Material->ParamBuffer || Material->ParamBuffer->GetBufferSize() != sizeof(FMaterialParams)) {
		Material->ParamBuffer = RenderCommand::CreateConstantBuffer(sizeof(FMaterialParams));
	}

	const float TotalTime{ EngineTimer::GetTotalTime() };

	FMaterialParams Params{};
	Params.BaseColor = ColorWorkflow::ToShaderColor(Material->BaseColor);
	Params.BaseColor.R *= ColorMultiplier.R;
	Params.BaseColor.G *= ColorMultiplier.G;
	Params.BaseColor.B *= ColorMultiplier.B;
	Params.BaseColor.A *= ColorMultiplier.A;
	Params.UVScrollOffset = Material->UVScrollSpeed * TotalTime;
	Params.bOpaque = Material->BlendState == EBlendState::Opaque ? 1u : 0u;
	Params.Shininess = Material->Shininess;
	const FLinearColor Specular{ ColorWorkflow::ToShaderColor(Material->SpecularColor) };
	Params.SpecularColor = FVector{ Specular.R, Specular.G, Specular.B };
	Params.bFlipNormalGreen = Material->bFlipNormalGreen ? 1u : 0u;
	Params.TextureBindFlags = HasBoundNormalMap(Material) ? MTBF_NormalMap : 0u;

	// 수신 머티리얼의 색상 합성 방식을 셰이더 상수로 전달
	Params.DecalColorBlendMode = static_cast<uint32>(Material->DecalColorBlendMode);

	RenderCommand::UpdateBufferData(Material->ParamBuffer.get(), &Params, sizeof(FMaterialParams));
	// Gouraud는 VS에서 스페큘러를 계산하므로 Shininess가 VS에도 필요하다.
	RenderCommand::BindConstantBuffer(1, Material->ParamBuffer.get(), EShaderBindFlagBits::Vertex | EShaderBindFlagBits::Pixel);
}

// b0 MVP 채우고 꽂기
void FRenderer::UpdatePerObjectConstants(const FRenderPacket& RenderPacket)
{
	// 빌보드·파티클처럼 Proxy 없이 제출된 패킷은 수신 플래그를 기록하지 않는다.
	const uint32 ObjectMask = RenderPacket.Proxy ? RenderPacket.Proxy->GetObjectMask() : 0u;
	UpdatePerObjectConstants(GetPacketWorld(RenderPacket), GetPacketNormalMatrix(RenderPacket), ObjectMask);
}

void FRenderer::UpdatePerObjectConstants(const FMatrix& World, const FMatrix& NormalMatrix, uint32 ObjectMask)
{
	FPerObjectConstants Constants{};
	Constants.World = World;
	Constants.NormalMatrix = NormalMatrix;
	Constants.ObjectMask = ObjectMask;

	RenderCommand::UpdateBufferData(PerObjectCB.get(), &Constants);
}

void FRenderer::EnsureTileLightCapacity(const FMatrix& ViewProjection, uint32 Width, uint32 Height, bool bVisualizeLightCount)
{
	if (TileLightCulling.EnsureTileCapacity(Width, Height))
		TileLightCulling.Dispatch(LocalLight, ViewProjection, Width, Height, bVisualizeLightCount);

}

void FRenderer::RenderShadowPass(
	const FShadowInfo* DirectionalShadow,
	const FVector& LightDirection,
	const FVector& CameraPosition,
	const FVector& CameraForward,
	float FovDegrees,
	float AspectRatio,
	TArray<FShadowInfo>& ShadowInfos,
	const TArray<FPrimitiveSceneProxy*>& Casters,
	FLightSceneProxy* PreviewShadowMapProxy,
	FShadowStats& ShadowStats,
	bool bVisualizeCascades,
	bool bCacheHit,
	bool bDrawSkeletalMeshes)
{
	SCOPE_CYCLE_COUNTER(STAT_Shadow);

	//이전 pass에서의 바인딩을 끊는다.
	ShadowMapManager.UnbindForShading();

	// Shadow Atlas 를 계산한다.
	ShadowMapManager.BuildAtlas(ShadowInfos);

	// Directional 상수 버퍼 준비 (카메라 기준 4개 외접구 및 Light VP 행렬 계산 후 b5 업로드)
	if (DirectionalShadow != nullptr)
	{
		ShadowMapManager.SetupDirectionalShadow(*DirectionalShadow, LightDirection, CameraPosition, CameraForward, FovDegrees, AspectRatio, bVisualizeCascades);
	}

	// Local Light들 준비
	ShadowMapManager.UploadShadowInfos(ShadowInfos);

	ID3D11DeviceContext* Context = RenderCommand::GetContext();

#if 0
	//공통 파이프라인 상태 (층마다 바뀌지 않음)
	FShaderProgram* DepthShader = FRenderResourceManager::GetShaderProgram("Resources/Shader/ShadowMap.hlsl");

	//VS만 바인딩
	Context->VSSetShader(DepthShader->VertexShader->GetShader(), nullptr, 0);
	Context->IASetInputLayout(DepthShader->VertexShader->GetLayout());
	Context->PSSetShader(nullptr, nullptr, 0);

	RenderCommand::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	RenderCommand::SetRasterizerState(ERasterizerState::ShadowDepth);

	// Depth on, LESS, Write
	RenderCommand::SetDepthStencilState(EDepthStencilState::Default);
	RenderCommand::SetBlendState(EBlendState::Opaque);
#endif

	RenderCommand::BindConstantBuffer(0, ShadowViewCB.get(), EShaderBindFlagBits::Vertex);
	RenderCommand::BindConstantBuffer(2, PerObjectCB.get(), EShaderBindFlagBits::Vertex);

	// 아틀라스
	// 캐스터 메쉬들을 그리는 공통 람다
	BuildShadowStaticGroups(Casters);
	auto DrawCasters = [&]() {
		DrawShadowStaticGroups(ShadowStats);
		for (FPrimitiveSceneProxy* Proxy : Casters)
		{
			if (!Proxy || !Proxy->IsVisible() || !Proxy->GetRenderState() || (!bDrawSkeletalMeshes && Proxy->IsUsingSkinning()))
			{
				continue;
			}

			if (!Proxy->IsCastShadow()) {
				ShadowStats.NumSkippedCasterDraws++;
				continue;
			}

			const FCachedMeshLOD& LOD = Proxy->GetLOD(0);
			if (!LOD.VertexBufferCount || !LOD.IndexBuffer)
			{
				continue;
			}
			if (mShadowInstancesReady && !IsSkeletalProxy(Proxy) && LOD.VertexBufferCount == 1 && LOD.VertexBuffers[0] && LOD.VertexBuffers[0]->GetStride() == sizeof(FVertexPNCT)) {
				continue;
			}

			UpdatePerObjectConstants(Proxy->GetLocalToWorld(), Proxy->GetNormalMatrix(), Proxy->GetObjectMask());

			if (Proxy->IsUsingSkinning())
			{
				RenderCommand::BindPipelineState(ShadowSkinningPipeline);
				RenderCommand::BindShaderResource(0, Proxy->GetBoneMatricesSRV(), EShaderBindFlagBits::Vertex);
			}
			else
			{
				RenderCommand::BindPipelineState(ShadowStaticPipeline);
			}

			RenderCommand::BindVertexBuffers(LOD.VertexBuffers, LOD.VertexBufferCount);
			RenderCommand::BindIndexBuffer(LOD.IndexBuffer);

			for (uint32 i = 0; i < LOD.NumSections; ++i)
			{
				const FCachedMeshSection& Section = Proxy->GetSection(LOD.FirstSection + i);
				if (Section.Material && Section.Material->BlendState != EBlendState::Opaque)
				{
					continue;
				}
				RenderCommand::DrawIndexed(Section.IndexCount, Section.StartIndex);
				ShadowStats.NumShadowDrawCalls++;
			}
		}
	};

	if (DirectionalShadow != nullptr)
	{
		// 4개의 캐스케이드를 4096 Atlas의 2x2 분할 영역에 각각 렌더
		for (uint32 Cascade = 0; Cascade < 4; ++Cascade)
		{
			ShadowMapManager.BeginDirectionalCascadeRender(Cascade);
			const FMatrix& CascadeVP = ShadowMapManager.GetCascadeMatrix(Cascade);
			RenderCommand::UpdateBufferData(ShadowViewCB.get(), &CascadeVP, sizeof(FMatrix));
			DrawCasters();
		}
		ShadowMapManager.EndDirectionalShadowRender();
	}

	if (!bCacheHit)
	{
		ShadowMapManager.ClearAtlas();

		//층(라이트)마다
		for (int32 Slice = 0; Slice < ShadowInfos.Num(); ++Slice)
		{
			//렌더 전 개별 바인딩
			ShadowMapManager.BeginShadowRenderWithAtlas(ShadowInfos[Slice]);


			//라이트마다의 VP 행렬 Map Unmap
			RenderCommand::UpdateBufferData(ShadowViewCB.get(), &ShadowInfos[Slice].LightViewProjection, sizeof(FMatrix));

			DrawCasters();
		}

		//DSV 해제
		if (ShadowInfos.Num() > 0)
		{
			ShadowMapManager.EndShadowPass();
		}
	}

	// TODO : 아틀라스 프리뷰
	//Preview 인덱스의 ShadowMap 서브 리소스를 복사한다.
	if (PreviewShadowMapProxy != nullptr)
	{
		ShadowMapManager.UpdatePreview(PreviewShadowMapProxy, ShadowInfos);
	}

	if (!bCacheHit) {
		// 다 새로 그리기
		int32 RenderedCount = ShadowInfos.Num();
		if (DirectionalShadow != nullptr)
		{
			RenderedCount += 4;
		}
		ShadowStats.NumShadowMapsRendered = RenderedCount;
		ShadowStats.NumCachedShadowMaps = 0;
	}
	else {
		// 재사용
		int32 RenderedCount = 0;
		if (DirectionalShadow != nullptr)
		{
			RenderedCount += 4;
		}
		ShadowStats.NumShadowMapsRendered = RenderedCount;
		ShadowStats.NumCachedShadowMaps = ShadowInfos.Num();
	}

	int64 TotalBytes = (int64)ShadowMapManager.GetAtlasResolution() * ShadowMapManager.GetAtlasResolution() * 4 * ShadowMapManager.GetNumAtlas();
	if (ShadowMapManager.GetDirectionalShadowSRV() != nullptr)
	{
		TotalBytes += (int64)ShadowMapManager.GetDirectionalResolution() * ShadowMapManager.GetDirectionalResolution() * 4;
	}

	ShadowStats.ShadowMapMemoryBytes = TotalBytes;
}

void FRenderer::BindShadowForShading()
{
	ShadowMapManager.BindForShading();
}
