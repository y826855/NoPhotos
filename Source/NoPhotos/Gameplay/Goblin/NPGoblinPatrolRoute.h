#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "NPGoblinPatrolRoute.generated.h"

class USplineComponent;

/**
 * 레벨 디자이너가 고블린의 순회 경로를 그릴 때 사용하는 폐곡선 Spline 액터입니다.
 * Spline은 목표 위치만 제공하며 실제 이동과 장애물 회피는 NavMesh/AIController가 담당합니다.
 */
UCLASS(Blueprintable)
class NOPHOTOS_API ANPGoblinPatrolRoute : public AActor
{
	GENERATED_BODY()

public:
	ANPGoblinPatrolRoute();

	UFUNCTION(BlueprintPure, Category = "Goblin Route")
	USplineComponent* GetSpline() const { return PatrolSpline; }

	UFUNCTION(BlueprintPure, Category = "Goblin Route")
	bool SupportsRouteGroup(FGameplayTag InRouteGroup) const;

	/** 두 개 이상의 점과 유효한 길이를 가진 폐곡선인지 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Goblin Route")
	bool IsUsableRoute() const;

	float FindDistanceClosestToWorldLocation(const FVector& WorldLocation) const;
	float AdvanceDistance(float Distance, float DeltaDistance) const;
	FVector GetWorldLocationAtDistance(float Distance) const;
	FVector GetWorldDirectionAtDistance(float Distance) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goblin Route")
	TObjectPtr<USplineComponent> PatrolSpline;

	/** 같은 그룹의 고블린 이벤트만 이 경로를 사용합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Goblin Route")
	FGameplayTag RouteGroup;
};
