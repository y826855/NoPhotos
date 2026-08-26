#include "NPMapEventSpawnPoint.h"

#include "Components/ArrowComponent.h"

ANPMapEventSpawnPoint::ANPMapEventSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	LocationMarker = CreateDefaultSubobject<UArrowComponent>(TEXT("LocationMarker"));
	SetRootComponent(LocationMarker);
	LocationMarker->SetArrowColor(FColor(80, 220, 120));
	LocationMarker->SetArrowSize(1.5f);
	LocationMarker->SetHiddenInGame(true);
	LocationMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool ANPMapEventSpawnPoint::SupportsSpawnGroup(const FGameplayTag SpawnGroup) const
{
	return bEnabled
		&& SpawnGroup.IsValid()
		&& SupportedSpawnGroups.HasTagExact(SpawnGroup);
}
