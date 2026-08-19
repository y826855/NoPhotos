#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Character/NPStablePhysicsPawn.h"
#include "NPRStablePhysicsPawn.generated.h"

class UPrimitiveComponent;
class FLifetimeProperty;

USTRUCT()
struct FReplicatedStableGrabState
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<AActor> GrabbedActor = nullptr;

	UPROPERTY()
	FName GrabbedComponentName = NAME_None;

	UPROPERTY()
	FName GrabbedBoneName = NAME_None;

	UPROPERTY()
	FTransform ConstraintFrame1 = FTransform::Identity;

	UPROPERTY()
	FTransform ConstraintFrame2 = FTransform::Identity;
};

/** 서버 권한으로 이동과 그랩 물리를 처리하는 Stable Physics Pawn입니다. */
UCLASS()
class NOPHOTOS_API ANPRStablePhysicsPawn : public ANPStablePhysicsPawn
{
	GENERATED_BODY()

public:
	ANPRStablePhysicsPawn();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Network|Grab")
	bool IsReplicatedRightHandActive() const { return bReplicatedRightHandActive; }

	UFUNCTION(BlueprintPure, Category="Network|Grab")
	bool IsReplicatedGrabActive() const { return IsValid(ReplicatedGrabState.GrabbedActor); }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void ApplyMoveInput(const FVector& WorldMoveInput) override;
	virtual void ApplyJumpRequest() override;
	virtual void ApplyRightHandState(bool bActive) override;
	virtual FRotator GetTargetViewRotation() const override;

private:
	/** 자주 변경되는 이동 입력은 유실을 허용하는 RPC로 전달합니다. */
	UFUNCTION(Server, Unreliable)
	void ServerSetMoveInput(FVector_NetQuantizeNormal WorldMoveInput);

	/** 현재 카메라 회전을 서버 권한 캐릭터 제어에 전달합니다. */
	UFUNCTION(Server, Unreliable)
	void ServerSetViewRotation(uint16 CompressedYaw, uint16 CompressedPitch);

	/** 정지 입력이 유실되어 서버에서 계속 움직이지 않도록 Reliable로 전달합니다. */
	UFUNCTION(Server, Reliable)
	void ServerStopMove();

	UFUNCTION(Server, Reliable)
	void ServerRequestJump();

	UFUNCTION(Server, Reliable)
	void ServerSetRightHandActive(bool bActive);

	UFUNCTION()
	void OnRep_RightHandActive();

	UFUNCTION()
	void OnRep_GrabState();

	void HandleGrabbedComponentChanged(UPrimitiveComponent* NewGrabbedComponent);
	UPrimitiveComponent* ResolveReplicatedGrabbedComponent() const;
	void UpdateViewRotationReplication(float DeltaSeconds);
	void SetReplicatedViewRotation(const FRotator& NewViewRotation);
	void SetServerRightHandState(bool bActive);
	void DrawGrabNetworkDebug() const;

	/** 다른 클라이언트에서도 오른손 IK 상태를 동일하게 표시하기 위한 값입니다. */
	UPROPERTY(ReplicatedUsing=OnRep_RightHandActive)
	bool bReplicatedRightHandActive = false;

	UPROPERTY(ReplicatedUsing=OnRep_GrabState)
	FReplicatedStableGrabState ReplicatedGrabState;

	/** 서버와 클라이언트 손 위치 차이를 확인하기 위한 디버그 값입니다. */
	UPROPERTY(Replicated)
	FVector_NetQuantize10 ReplicatedServerHandWorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="Network|Grab Debug")
	bool bDrawGrabNetworkDebug = true;

	// 클라이언트 애니메이션에 사용할 서버의 이동 속도입니다.
	UPROPERTY(Replicated)
	FVector_NetQuantize10 ReplicatedAnimationVelocity = FVector::ZeroVector;

	// 클라이언트 애니메이션에 사용할 서버의 이동 가속도입니다.
	UPROPERTY(Replicated)
	FVector_NetQuantize10 ReplicatedAnimationAcceleration = FVector::ZeroVector;

	// 클라이언트 애니메이션에 사용할 서버의 낙하 상태입니다.
	UPROPERTY(Replicated)
	bool bReplicatedAnimationIsFalling = true;

	// 클라이언트 애니메이션에 사용할 서버의 정면 방향입니다.
	UPROPERTY(Replicated)
	FVector_NetQuantizeNormal ReplicatedAnimationForwardDirection = FVector::ForwardVector;

	/** 서버 물리와 비소유 클라이언트의 손/허리 표현에 사용할 시점 회전입니다. */
	UPROPERTY(Replicated)
	FRotator ReplicatedViewRotation = FRotator::ZeroRotator;

	bool bClientWasMoving = false;
	float ViewRotationSendAccumulator = 0.05f;
};
