#include "NPGoblinPatrolRoute.h"

#include "Components/SplineComponent.h"

ANPGoblinPatrolRoute::ANPGoblinPatrolRoute()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	PatrolSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PatrolSpline"));
	SetRootComponent(PatrolSpline);
	PatrolSpline->SetClosedLoop(true);
	PatrolSpline->SetDrawDebug(true);

	RouteGroup = FGameplayTag::RequestGameplayTag(FName(TEXT("Goblin")), false);
}

bool ANPGoblinPatrolRoute::SupportsRouteGroup(const FGameplayTag InRouteGroup) const
{
	return InRouteGroup.IsValid() && RouteGroup == InRouteGroup;
}

bool ANPGoblinPatrolRoute::IsUsableRoute() const
{
	return IsValid(PatrolSpline)
		&& PatrolSpline->IsClosedLoop()
		&& PatrolSpline->GetNumberOfSplinePoints() >= 2
		&& PatrolSpline->GetSplineLength() > KINDA_SMALL_NUMBER;
}

float ANPGoblinPatrolRoute::FindDistanceClosestToWorldLocation(
	const FVector& WorldLocation) const
{
	if (!IsUsableRoute())
	{
		return 0.0f;
	}

	const float InputKey = PatrolSpline->FindInputKeyClosestToWorldLocation(WorldLocation);
	return PatrolSpline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
}

float ANPGoblinPatrolRoute::AdvanceDistance(
	const float Distance,
	const float DeltaDistance) const
{
	if (!IsUsableRoute())
	{
		return 0.0f;
	}

	const float SplineLength = PatrolSpline->GetSplineLength();
	return FMath::Fmod(FMath::Fmod(Distance + DeltaDistance, SplineLength) + SplineLength, SplineLength);
}

FVector ANPGoblinPatrolRoute::GetWorldLocationAtDistance(const float Distance) const
{
	if (!IsUsableRoute())
	{
		return GetActorLocation();
	}

	return PatrolSpline->GetLocationAtDistanceAlongSpline(
		AdvanceDistance(Distance, 0.0f),
		ESplineCoordinateSpace::World);
}

FVector ANPGoblinPatrolRoute::GetWorldDirectionAtDistance(const float Distance) const
{
	if (!IsUsableRoute())
	{
		return GetActorForwardVector();
	}

	return PatrolSpline->GetDirectionAtDistanceAlongSpline(
		AdvanceDistance(Distance, 0.0f),
		ESplineCoordinateSpace::World);
}
