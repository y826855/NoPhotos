#pragma once

#include "CoreMinimal.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "NPStablePhysicsGrabComponent.generated.h"

class UPrimitiveComponent;
class USkeletalMeshComponent;
class UGrabbableComponent;

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
	void SetReplicatedGrabFrameBlendDuration(float InBlendDuration);
	void SetGrabRetryCooldown(float InRetryCooldown);
	void SetMovementIntent(const FVector& WorldMovementIntent);
	void NotifyJumpIntent();

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category="Grab", meta=(ClampMin="0.0"))
	float GrabRadius = 18.0f;

	float GrabLinearBreakThreshold = 200000.0f;

	float ReplicatedGrabFrameBlendDuration = 0.15f;
	float GrabRetryCooldown = 0.5f;

	UPROPERTY(EditAnywhere, Category="Grab Intent", meta=(ClampMin="0.0"))
	float JumpIntentDuration = 0.15f;

#pragma region Grab Debug Settings
	UPROPERTY(EditAnywhere, Category="Grab Debug")
	bool bDrawGrabDebug = true;

	UPROPERTY(EditAnywhere, Category="Grab Debug", meta=(ClampMin="0.0"))
	float GrabDebugMinimumForce = 100.0f;

	UPROPERTY(EditAnywhere, Category="Grab Debug", meta=(ClampMin="0.0"))
	float GrabDebugForceSmoothingSpeed = 8.0f;

	UPROPERTY(EditAnywhere, Category="Grab Debug", meta=(ClampMin="0.0"))
	float GrabDebugIntentSmoothingSpeed = 12.0f;
#pragma endregion

private:
	UFUNCTION()
	void HandleConstraintBroken(int32 ConstraintIndex);

	void TryGrab();
	bool Grab(
		UPrimitiveComponent* PrimitiveComponent,
		UGrabbableComponent* GrabbableComponent);
	void UpdateGrabForce(float DeltaTime);
	void UpdateReplicatedGrabFrameBlend(float DeltaTime);
	void ReleaseGrab();

	UPROPERTY(Transient)
	USkeletalMeshComponent* PhysicsMesh = nullptr;

	UPROPERTY(Transient)
	UPrimitiveComponent* GrabbedComponent = nullptr;

	UPROPERTY(Transient)
	UGrabbableComponent* GrabbedGrabbableComponent = nullptr;

	FName HandBoneName = NAME_None;
	FName GrabbedBoneName = NAME_None;
	bool bGrabSimulationEnabled = true;
	bool bGrabRequested = false;
	bool bWaitForGrabRelease = false;
	bool bReplicatedGrabFrameBlendActive = false;
	FVector MovementIntent = FVector::ZeroVector;
	float JumpIntentRemainingTime = 0.0f;
	float GrabRetryCooldownRemaining = 0.0f;
	float ReplicatedGrabFrameBlendElapsed = 0.0f;
	FTransform ReplicatedGrabFrameBlendStart = FTransform::Identity;
	FTransform ReplicatedGrabFrameBlendTarget = FTransform::Identity;

#pragma region Grab Debug Functions
	void ResetGrabDebug();
	void UpdateGrabDebug(
		float DeltaTime,
		const FVector& RelicForce,
		const FVector& UserIntent);
	void DrawGrabDebug() const;
	void DrawGrabForceDebug(const FVector& ForceStart) const;
#pragma endregion

#pragma region Grab Debug State
	FVector LastDebugRelicForce = FVector::ZeroVector;
	FVector SmoothedDebugUserIntent = FVector::ZeroVector;
	float LastDebugIntentForceAlignment = 0.0f;
#pragma endregion
};
