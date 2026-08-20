#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPPulleyBarrierGimmick.generated.h"

class UGrabbableComponent;
class UNPPulleyBarrierGimmickComponent;
class UPhysicsConstraintComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class FLifetimeProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnPulleyTravelChanged,
	float,
	NormalizedTravel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPulleyBarrierOpened);
DECLARE_MULTICAST_DELEGATE(FOnPulleyHandleGrabbed);

UCLASS(Blueprintable)
class NOPHOTOS_API ANPPulleyBarrierGimmick : public AActor
{
	GENERATED_BODY()

public:
	ANPPulleyBarrierGimmick();
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Pulley Barrier")
	float GetNormalizedTravel() const;

	UFUNCTION(BlueprintPure, Category="Pulley Barrier")
	bool IsOpened() const;

	UPROPERTY(BlueprintAssignable, Category="Pulley Barrier")
	FOnPulleyTravelChanged OnTravelChanged;

	UPROPERTY(BlueprintAssignable, Category="Pulley Barrier")
	FOnPulleyBarrierOpened OnBarrierOpened;

	FOnPulleyHandleGrabbed OnHandleGrabbed;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> HandleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPhysicsConstraintComponent> HandleConstraint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> BarrierMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> LeftPulleyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> RightPulleyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UGrabbableComponent> GrabbableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UNPPulleyBarrierGimmickComponent> GimmickComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier", meta=(ClampMin="1.0"))
	float HandleTravelDistance = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier", meta=(ClampMin="0.0"))
	float BarrierTravelMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier", meta=(ClampMin="0.0"))
	float CounterweightMass = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier", meta=(ClampMin="0.0"))
	float HandleLinearDamping = 5.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category="Pulley Barrier",
		meta=(ClampMin="0.0", Units="cm/s"))
	float MaxHandleDownwardSpeed = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier|Return", meta=(ClampMin="1.0"))
	float ReturnForceRampDistance = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier|Return", meta=(ClampMin="0.0"))
	float ReturnDampingRatio = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier|Return", meta=(ClampMin="0.0"))
	float SettlingDistance = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier|Return", meta=(ClampMin="0.0"))
	float SettlingSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier", meta=(ClampMin="1.0"))
	float PulleyRadius = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier")
	FVector PulleyRotationAxis = FVector::RightVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier")
	float LeftPulleyRotationDirection = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier")
	float RightPulleyRotationDirection = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pulley Barrier", meta=(ClampMin="0.0", ClampMax="1.0"))
	float OpenTravelRatio = 0.95f;

private:
	void ConfigureHandleConstraint();
	float GetSignedHandleTravelDistance() const;
	void ApplyHandleReturnForce(float SignedTravelDistance);
	void LimitHandleDownwardSpeed();
	bool TrySettleHandle(float SignedTravelDistance);
	void HandleGrabStarted(UPrimitiveComponent* GrabbedComponent);
	void UpdateTravelFromHandle();
	void ApplyTravel(float TravelDistance);
	void ApplyPulleyRotation(
		UStaticMeshComponent* PulleyMesh,
		const FQuat& InitialRotation,
		float TravelDistance,
		float RotationDirection);
	void HandleGimmickCompleted();

	UFUNCTION()
	void OnRep_TravelDistance();

	UPROPERTY(ReplicatedUsing=OnRep_TravelDistance)
	float ReplicatedTravelDistance = 0.0f;

	FVector InitialHandleRelativeLocation = FVector::ZeroVector;
	FVector InitialHandleWorldLocation = FVector::ZeroVector;
	FVector InitialBarrierRelativeLocation = FVector::ZeroVector;
	FQuat InitialLeftPulleyRelativeRotation = FQuat::Identity;
	FQuat InitialRightPulleyRelativeRotation = FQuat::Identity;
	bool bInitialTransformsCached = false;
};
