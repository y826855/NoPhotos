#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NPGoblinCharacter.generated.h"

class ANPGoblinPatrolRoute;

/**
 * NavMesh를 이용해 배회하고 가까운 플레이어에게서 도망가는 고블린입니다.
 * 실제 이동 의사결정은 ANPGoblinAIController가 서버에서 수행합니다.
 */
UCLASS(Blueprintable)
class NOPHOTOS_API ANPGoblinCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANPGoblinCharacter();

	/** 이벤트가 생성 직전에 주입하는 서버 전용 순찰 경로입니다. */
	void SetPatrolRoute(ANPGoblinPatrolRoute* InPatrolRoute) { PatrolRoute = InPatrolRoute; }
	ANPGoblinPatrolRoute* GetPatrolRoute() const { return PatrolRoute; }

	float GetAIDecisionInterval() const { return FMath::Max(0.05f, AIDecisionInterval); }
	float GetPatrolAcceptanceRadius() const { return FMath::Max(1.0f, PatrolAcceptanceRadius); }
	float GetPatrolTargetSpacing() const { return FMath::Max(GetPatrolAcceptanceRadius() * 2.0f, PatrolTargetSpacing); }
	float GetPatrolTargetSwitchDistance() const
	{
		return FMath::Clamp(
			PatrolTargetSwitchDistance,
			GetPatrolAcceptanceRadius() + 1.0f,
			GetPatrolTargetSpacing() - 1.0f);
	}
	float GetRouteReturnLookAheadDistance() const { return FMath::Max(0.0f, RouteReturnLookAheadDistance); }
	float GetFleeRouteDirectionWeight() const { return FMath::Max(0.0f, FleeRouteDirectionWeight); }
	float GetFleeRouteDistancePenaltyWeight() const { return FMath::Max(0.0f, FleeRouteDistancePenaltyWeight); }
	float GetRoamRadius() const { return FMath::Max(0.0f, RoamRadius); }
	float GetRoamAcceptanceRadius() const { return FMath::Max(1.0f, RoamAcceptanceRadius); }
	float GetMinimumRoamWaitTime() const { return FMath::Max(0.0f, MinimumRoamWaitTime); }
	float GetMaximumRoamWaitTime() const { return FMath::Max(0.0f, MaximumRoamWaitTime); }
	float GetPlayerDetectionRadius() const { return FMath::Max(0.0f, PlayerDetectionRadius); }
	float GetFleeReleaseDistance() const { return FMath::Max(GetPlayerDetectionRadius(), FleeReleaseDistance); }
	float GetFleeTravelDistance() const { return FMath::Max(1.0f, FleeTravelDistance); }
	float GetFleeAcceptanceRadius() const { return FMath::Max(1.0f, FleeAcceptanceRadius); }
	float GetFleeRepathInterval() const { return FMath::Max(0.05f, FleeRepathInterval); }
	int32 GetFleeSamplingAttempts() const { return FMath::Max(1, FleeSamplingAttempts); }
	float GetRoamMoveSpeed() const { return FMath::Max(0.0f, RoamMoveSpeed); }
	float GetFleeMoveSpeed() const { return FMath::Max(0.0f, FleeMoveSpeed); }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI", meta = (ClampMin = "0.05", Units = "s"))
	float AIDecisionInterval = 0.25f;

	/** 순찰 중 한 번의 MoveTo가 Spline을 따라 전진할 거리입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Patrol", meta = (ClampMin = "100.0", Units = "cm"))
	float PatrolTargetSpacing = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Patrol", meta = (ClampMin = "1.0", Units = "cm"))
	float PatrolAcceptanceRadius = 100.0f;

	/** 현재 목표에 완전히 멈추기 전에 다음 Spline 목표로 전환할 거리입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Patrol", meta = (ClampMin = "1.0", Units = "cm"))
	float PatrolTargetSwitchDistance = 250.0f;

	/** 도주 종료 후 가장 가까운 Spline 위치보다 앞쪽으로 복귀할 거리입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Patrol", meta = (ClampMin = "0.0", Units = "cm"))
	float RouteReturnLookAheadDistance = 400.0f;

	/** 최초 생성 위치를 중심으로 랜덤 배회 목적지를 찾을 반경입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Roam", meta = (ClampMin = "0.0", Units = "cm"))
	float RoamRadius = 1500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Roam", meta = (ClampMin = "1.0", Units = "cm"))
	float RoamAcceptanceRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Roam", meta = (ClampMin = "0.0", Units = "s"))
	float MinimumRoamWaitTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Roam", meta = (ClampMin = "0.0", Units = "s"))
	float MaximumRoamWaitTime = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Roam", meta = (ClampMin = "0.0", Units = "cm/s"))
	float RoamMoveSpeed = 300.0f;

	/** 플레이어가 이 거리 안으로 들어오면 도망 상태로 전환합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Flee", meta = (ClampMin = "0.0", Units = "cm"))
	float PlayerDetectionRadius = 1000.0f;

	/** 이 거리까지 벗어난 뒤 배회 상태로 돌아갑니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Flee", meta = (ClampMin = "0.0", Units = "cm"))
	float FleeReleaseDistance = 1400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Flee", meta = (ClampMin = "1.0", Units = "cm"))
	float FleeTravelDistance = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Flee", meta = (ClampMin = "1.0", Units = "cm"))
	float FleeAcceptanceRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Flee", meta = (ClampMin = "0.05", Units = "s"))
	float FleeRepathInterval = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Flee", meta = (ClampMin = "1", UIMin = "1"))
	int32 FleeSamplingAttempts = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Flee", meta = (ClampMin = "0.0", Units = "cm/s"))
	float FleeMoveSpeed = 600.0f;

	/** 도주 후보가 Spline 진행 방향과 일치할 때 주는 가산점 비율입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Flee", meta = (ClampMin = "0.0"))
	float FleeRouteDirectionWeight = 0.5f;

	/** 도주 후보가 Spline에서 멀어질수록 적용할 감점 비율입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goblin|AI|Flee", meta = (ClampMin = "0.0"))
	float FleeRouteDistancePenaltyWeight = 0.25f;

private:
	UPROPERTY(Transient)
	TObjectPtr<ANPGoblinPatrolRoute> PatrolRoute;
};
