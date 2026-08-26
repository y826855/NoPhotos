#include "Gameplay/Relic/Gimmick/NPRelicCaseKey.h"

#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "Gameplay/Interaction/Components/GrabbableComponent.h"

ANPRelicCaseKey::ANPRelicCaseKey()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	KeyCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("KeyCollision"));
	SetRootComponent(KeyCollision);
	KeyCollision->SetBoxExtent(FVector(10.0f));
	KeyCollision->SetMobility(EComponentMobility::Movable);
	KeyCollision->SetCollisionProfileName(
		UCollisionProfile::PhysicsActor_ProfileName);
	KeyCollision->SetSimulatePhysics(true);
	KeyCollision->SetIsReplicated(true);

	GrabbableComponent = CreateDefaultSubobject<UGrabbableComponent>(
		TEXT("GrabbableComponent"));
}

void ANPRelicCaseKey::NotifyUnlockSucceeded()
{
	if (HasAuthority())
	{
		MulticastUnlockSucceeded();
	}
}

void ANPRelicCaseKey::MulticastUnlockSucceeded_Implementation()
{
	OnUnlockSucceeded();
}
