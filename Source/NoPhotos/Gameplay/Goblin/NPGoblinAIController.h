#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "NPGoblinAIController.generated.h"

class ANPGoblinCharacter;
class ANPGoblinPatrolRoute;

enum class ENPGoblinMovementState : uint8
{
	Patrol,
	Flee,
	ReturnToRoute,
	RandomRoamFallback
};

/** 서버에서 고블린의 NavMesh 배회와 플레이어 회피 목적지를 선택합니다. */
UCLASS()
class NOPHOTOS_API ANPGoblinAIController : public AAIController
{
	GENERATED_BODY()

public:
	ANPGoblinAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	void EvaluateMovement();
	void EnterFleeState();
	void LeaveFleeState();
	void BeginReturnToRoute();
	void TryFollowPatrolRoute();
	void TryReturnToRoute();
	void TryStartRoaming();
	void TryUpdateFleeDestination(const TArray<FVector>& PlayerLocations);
	bool GatherPlayerLocations(TArray<FVector>& OutPlayerLocations, float& OutNearestDistanceSquared) const;
	bool FindBestFleeDestination(const TArray<FVector>& PlayerLocations, FVector& OutDestination) const;
	bool RequestMoveToLocation(const FVector& Destination, float AcceptanceRadius);
	ANPGoblinCharacter* GetGoblin() const;
	ANPGoblinPatrolRoute* GetUsablePatrolRoute() const;

	FVector RoamOrigin = FVector::ZeroVector;
	FVector ReturnTargetLocation = FVector::ZeroVector;
	FVector ActivePatrolTargetLocation = FVector::ZeroVector;
	float ActivePatrolTargetDistance = 0.0f;
	ENPGoblinMovementState MovementState = ENPGoblinMovementState::RandomRoamFallback;
	bool bHasActivePatrolTarget = false;
	bool bReturnMoveRequested = false;
	double NextRoamTime = 0.0;
	double NextFleeRepathTime = 0.0;
	FTimerHandle DecisionTimer;
};
