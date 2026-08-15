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

	/** false이면 탐색과 Constraint 생성을 수행하지 않습니다. */
	void SetGrabSimulationEnabled(bool bEnabled);
	void SetLinearBreakThreshold(float InLinearBreakThreshold);

	FOnStableGrabChanged OnGrabbedComponentChanged;

	UFUNCTION(BlueprintPure, Category="Grab")
	bool IsHoldingObject() const { return IsValid(GrabbedComponent); }

	UFUNCTION(BlueprintPure, Category="Grab")
	UPrimitiveComponent* GetGrabbedComponent() const { return GrabbedComponent; }

	FName GetGrabbedBoneName() const { return GrabbedBoneName; }
	FTransform GetGrabConstraintFrame(EConstraintFrame::Type Frame) const;
	void ApplyReplicatedGrab(
		UPrimitiveComponent* PrimitiveComponent,
		FName BoneName,
		const FTransform& Frame1,
		const FTransform& Frame2);
	void ClearReplicatedGrab();

protected:
	virtual void BeginPlay() override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category="Grab", meta=(ClampMin="0.0"))
	float GrabRadius = 18.0f;

	float GrabLinearBreakThreshold = 200000.0f;

	UPROPERTY(EditAnywhere, Category="Grab Debug")
	bool bDrawGrabDebug = true;

private:
	UFUNCTION()
	void HandleConstraintBroken(int32 ConstraintIndex);

	void TryGrab();
	void Grab(UPrimitiveComponent* PrimitiveComponent);
	void ReleaseGrab();
	void DrawGrabDebug() const;

	UPROPERTY(Transient)
	USkeletalMeshComponent* PhysicsMesh = nullptr;

	UPROPERTY(Transient)
	UPrimitiveComponent* GrabbedComponent = nullptr;

	FName HandBoneName = NAME_None;
	FName GrabbedBoneName = NAME_None;
	bool bGrabSimulationEnabled = true;
	bool bGrabRequested = false;
	bool bWaitForGrabRelease = false;
};
