#pragma once

#include "CoreMinimal.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "NPStablePhysicsGrabComponent.generated.h"

class UPrimitiveComponent;
class USkeletalMeshComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnStableGrabChanged, UPrimitiveComponent*);

/** 손 주변의 잡을 수 있는 물리 Body를 찾아 Physics Constraint로 연결합니다. */
UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class NOPHOTOS_API UNPStablePhysicsGrabComponent : public UPhysicsConstraintComponent
{
	GENERATED_BODY()

public:
	UNPStablePhysicsGrabComponent();

	/** 물리 캐릭터 메시와 그랩 기준으로 사용할 손 본을 설정합니다. */
	void Initialize(USkeletalMeshComponent* InPhysicsMesh, FName InHandBoneName);

	/** 입력을 누르는 동안 그랩을 시도하고, 입력이 끝나면 즉시 놓습니다. */
	void SetGrabRequested(bool bRequested);

	/** false이면 Constraint 생성을 수행하지 않습니다. 후보 탐색은 시각 IK를 위해 계속합니다. */
	void SetGrabSimulationEnabled(bool bEnabled);
	UFUNCTION(BlueprintPure, Category="Grab")
	bool HasGrabCandidate() const { return IsValid(CandidateComponent); }

	/** 후보 또는 잡은 물체의 표면 Grab Point를 월드 좌표로 반환합니다. */
	bool GetVisualGrabPoint(FVector& OutWorldPoint) const;

	FOnStableGrabChanged OnGrabbedComponentChanged;

	UFUNCTION(BlueprintPure, Category="Grab")
	bool IsHoldingObject() const { return IsValid(GrabbedComponent); }

	UFUNCTION(BlueprintPure, Category="Grab")
	UPrimitiveComponent* GetGrabbedComponent() const { return GrabbedComponent; }

	FName GetGrabbedBoneName() const { return GrabbedBoneName; }
	FVector GetGrabPointLocal() const { return GrabPointLocal; }
	FTransform GetGrabConstraintFrame(EConstraintFrame::Type Frame) const;
	void ApplyReplicatedGrab(
		UPrimitiveComponent* PrimitiveComponent,
		FName BoneName,
		const FTransform& Frame1,
		const FTransform& Frame2,
		const FVector& InGrabPointLocal);
	void ClearReplicatedGrab();

protected:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** 캐릭터 정면에서 후보를 찾는 영역의 전방 길이입니다. */
	UPROPERTY(EditAnywhere, Category="Grab|Search", meta=(ClampMin="0.0"))
	float FrontSearchDistance = 220.0f;

	UPROPERTY(EditAnywhere, Category="Grab|Search", meta=(ClampMin="0.0"))
	float FrontSearchHalfWidth = 90.0f;

	UPROPERTY(EditAnywhere, Category="Grab|Search", meta=(ClampMin="0.0"))
	float FrontSearchHalfHeight = 110.0f;

	/** PhysicsMesh 원점 기준 탐색 영역 중심의 높이 보정값입니다. */
	UPROPERTY(EditAnywhere, Category="Grab|Search")
	float FrontSearchVerticalOffset = -20.0f;

	UPROPERTY(EditAnywhere, Category="Grab", meta=(ClampMin="0.0"))
	float GrabAttachDistance = 25.0f;

	/** 0이면 질량 제한을 사용하지 않습니다. */
	UPROPERTY(EditAnywhere, Category="Grab", meta=(ClampMin="0.0"))
	float MaximumGrabMass = 50.0f;

	UPROPERTY(EditAnywhere, Category="Grab Debug")
	bool bDrawGrabDebug = true;

private:
	void UpdateCandidate();
	bool IsValidCandidate(
		UPrimitiveComponent* PrimitiveComponent,
		FName BoneName) const;
	void ClearCandidate();
	void Grab(UPrimitiveComponent* PrimitiveComponent, FName BoneName, const FVector& WorldGrabPoint);
	void ReleaseGrab();
	void DrawGrabDebug() const;

	UPROPERTY(Transient)
	USkeletalMeshComponent* PhysicsMesh = nullptr;

	UPROPERTY(Transient)
	UPrimitiveComponent* GrabbedComponent = nullptr;

	UPROPERTY(Transient)
	UPrimitiveComponent* CandidateComponent = nullptr;

	FName HandBoneName = NAME_None;
	FName GrabbedBoneName = NAME_None;
	FName CandidateBoneName = NAME_None;
	FVector CandidateGrabPoint = FVector::ZeroVector;
	FVector CandidateGrabPointLocal = FVector::ZeroVector;
	FVector GrabPointLocal = FVector::ZeroVector;
	bool bGrabSimulationEnabled = true;
	bool bGrabRequested = false;
};
