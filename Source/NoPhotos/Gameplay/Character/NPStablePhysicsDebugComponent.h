#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPStablePhysicsDebugComponent.generated.h"

class ANPStablePhysicsPawn;
class UNPStablePhysicsGrabComponent;

/** Stable Physics 캐릭터의 디버그 표시와 표시용 상태를 관리합니다. */
UCLASS(ClassGroup=(Physics))
class NOPHOTOS_API UNPStablePhysicsDebugComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPStablePhysicsDebugComponent();

	void ResetGrabDebug();
	void UpdateGrabDebug(
		const UNPStablePhysicsGrabComponent& GrabComponent,
		float DeltaTime,
		const FVector& RelicForce,
		const FVector& UserIntent);

protected:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	void DrawFacingDebug(const ANPStablePhysicsPawn& Pawn) const;
	void DrawPhysicalProfileDebug(const ANPStablePhysicsPawn& Pawn) const;
	void DrawPhysicalProfileBone(
		const ANPStablePhysicsPawn& Pawn,
		FName BoneName,
		const FColor& Color,
		float Rigidity,
		const FString& Label) const;
	void DrawPhysicalProfileLink(
		const ANPStablePhysicsPawn& Pawn,
		FName StartBoneName,
		FName EndBoneName,
		const FColor& Color,
		float Rigidity) const;
	void DrawGrabDebug(const UNPStablePhysicsGrabComponent& GrabComponent) const;
	void DrawGrabForceDebug(
		const UNPStablePhysicsGrabComponent& GrabComponent,
		const FVector& ForceStart) const;
	void DrawGrabNetworkDebug(const ANPStablePhysicsPawn& Pawn) const;

	FVector LastGrabRelicForce = FVector::ZeroVector;
	FVector SmoothedGrabUserIntent = FVector::ZeroVector;
	float LastGrabIntentForceAlignment = 0.0f;
};
