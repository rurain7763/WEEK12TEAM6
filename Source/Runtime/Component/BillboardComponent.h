#pragma once

#include "PrimitiveComponent.h"
#include "Render/Material.h"

struct FViewBasis;

enum class EBillboardAlignment
{
	FaceCamera,
	AxisAligned
};

enum class EBillboardAxisLock
{
	X, NegX,
	Y, NegY,
	Z, NegZ
};

class UBillboardComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UBillboardComponent, UPrimitiveComponent)

	REFLECT_START(ClassName)
		PROPERTY(Material)
	REFLECT_END()

public:
	UBillboardComponent();
	virtual ~UBillboardComponent() override;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime) override;

	virtual int32 GetNumMaterials() const override { return 1; }
	virtual UMaterial* GetMaterial(int32 SlotIndex) const override { return SlotIndex == 0 ? Material : nullptr; }
	virtual void SetMaterial(int32 SlotIndex, UMaterial* InMaterial) override { if (SlotIndex == 0) Material = InMaterial; }

	virtual const FModelMeshData* GetMeshData() const override { return QuadMesh ? &QuadMesh->GetMeshData() : nullptr; }
	virtual FBox CalcLocalBounds() const override;

	void SetTintColor(const FLinearColor& InColor) { SpriteParams.TintColor = InColor; bFollowLightColor = false; }
	void SetTintColor(const FColor& InColor) { SpriteParams.TintColor = InColor.ReinterpretAsLinear(); bFollowLightColor = false; }
	const FLinearColor& GetTintColor() const { return SpriteParams.TintColor; }

	void SetFollowLightColor(bool bFollow) { bFollowLightColor = bFollow; }
	bool IsFollowingLightColor() const { return bFollowLightColor; }

	void SetTexture(UTexture2D* InTexture);
	void SetTexture(const FString& TexturePath);
	const FString& GetTexturePath() const { return TexturePath; }

	void SetAligned(EBillboardAlignment InAligned);
	void SetAxisLock(EBillboardAxisLock InAxisLock);

	FMatrix GetBillboardMatrix(const FViewBasis& View) const;
	// View별 Billboard 행렬을 계산해 같은 렌더 패킷 형식으로 제출한다.
	void SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewBasis& View) override;

	virtual void Serialize(json& Handle, bool bIsLoading) override;

	using Super::Serialize;
	virtual void Serialize(FStructuredArchive::FRecord Record) override;

protected:
	EBillboardAlignment Aligned = EBillboardAlignment::FaceCamera;
	EBillboardAxisLock AxisLock = EBillboardAxisLock::X;
	
	UMaterial* Material = nullptr;
	UStaticMesh* QuadMesh = nullptr;
	FSpriteMaterialParams SpriteParams;
	FString TexturePath;

	bool bFollowLightColor = true;
};
