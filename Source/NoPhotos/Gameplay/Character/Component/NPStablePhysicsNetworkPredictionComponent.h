#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "NPStablePhysicsNetworkPredictionComponent.generated.h"

class FLifetimeProperty;
class UNPStablePhysicsGrabComponent;
class UNPStablePhysicsMovementComponent;
class USkeletalMeshComponent;

USTRUCT()
struct FNPStablePhysicsRootState
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize100 Position = FVector::ZeroVector;

	UPROPERTY()
	FQuat Rotation = FQuat::Identity;

	UPROPERTY()
	FVector_NetQuantize10 LinearVelocity = FVector::ZeroVector;

	UPROPERTY()
	FVector_NetQuantize10 AngularVelocity = FVector::ZeroVector;

	UPROPERTY()
	float ServerWorldTime = 0.0f;
};

/**
 * 소유 클라이언트는 입력을 즉시 예측하고, 서버는 같은 입력으로 권위 물리를 실행합니다.
 * 서버의 Root Body 상태는 소유 클라이언트에만 전달되어 부드러운 속도 보정에 사용됩니다.
 */
UCLASS(ClassGroup=(Network), meta=(BlueprintSpawnableComponent))
class NOPHOTOS_API UNPStablePhysicsNetworkPredictionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPStablePhysicsNetworkPredictionComponent();

	void Initialize(
		USkeletalMeshComponent* InPhysicsMesh,
		UNPStablePhysicsMovementComponent* InMovement,
		UNPStablePhysicsGrabComponent* InGrab,
		FName InRootBodyName);

	void SendMoveInput(const FVector& WorldMoveInput);
	void SendStopMove();
	void SendJumpRequest();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION(Server, Unreliable)
	void ServerSetMoveInput(
		uint16 InputSequence,
		FVector_NetQuantizeNormal WorldMoveInput);

	UFUNCTION(Server, Reliable)
	void ServerStopMove(uint16 InputSequence);

	UFUNCTION(Server, Reliable)
	void ServerRequestJump();

	UFUNCTION()
	void OnRep_ServerRootState();

	void CaptureServerRootState();
	void ApplyOwnerCorrection(float DeltaTime);
	void SendPendingMoveInput(float DeltaTime);
	bool IsNearUnheldDynamicBody(const FVector& RootPosition) const;
	bool AcceptInputSequence(uint16 InputSequence);
	float GetEstimatedServerWorldTime() const;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> PhysicsMesh = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UNPStablePhysicsMovementComponent> Movement = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UNPStablePhysicsGrabComponent> Grab = nullptr;

	UPROPERTY(ReplicatedUsing=OnRep_ServerRootState)
	FNPStablePhysicsRootState ServerRootState;

	UPROPERTY(EditAnywhere, Category="Network Prediction", meta=(ClampMin="0.0"))
	float PositionErrorTolerance = 2.0f;

	UPROPERTY(EditAnywhere, Category="Network Prediction", meta=(ClampMin="0.01"))
	float PositionCorrectionTime = 0.2f;

	UPROPERTY(EditAnywhere, Category="Network Prediction", meta=(ClampMin="0.0"))
	float MaximumCorrectionSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, Category="Network Prediction", meta=(ClampMin="0.0"))
	float MaximumCorrectionAcceleration = 1800.0f;

	UPROPERTY(EditAnywhere, Category="Network Prediction", meta=(ClampMin="0.0"))
	float HardSnapDistance = 250.0f;

	UPROPERTY(EditAnywhere, Category="Network Prediction", meta=(ClampMin="0.0"))
	float MaximumExtrapolationTime = 0.25f;

	UPROPERTY(EditAnywhere, Category="Network Prediction", meta=(ClampMin="0.0"))
	float MaximumServerStateAge = 0.4f;

	UPROPERTY(EditAnywhere, Category="Network Prediction|Dynamic Contact", meta=(ClampMin="0.0"))
	float DynamicBodyDetectionRadius = 80.0f;

	UPROPERTY(EditAnywhere, Category="Network Prediction|Dynamic Contact", meta=(ClampMin="0.0", ClampMax="1.0"))
	float DynamicBodyCorrectionScale = 0.2f;

	UPROPERTY(EditAnywhere, Category="Network Prediction", meta=(ClampMin="0.01"))
	float InputSendInterval = 1.0f / 30.0f;

	FName RootBodyName = NAME_None;
	FVector PendingMoveInput = FVector::ZeroVector;
	uint16 LocalInputSequence = 0;
	uint16 LastServerInputSequence = 0;
	float InputSendAccumulator = 1.0f / 30.0f;
	bool bHasReceivedInput = false;
	bool bHasServerRootState = false;
};
