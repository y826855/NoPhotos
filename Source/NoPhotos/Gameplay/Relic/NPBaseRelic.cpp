#include "Gameplay/Relic/NPBaseRelic.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Gameplay/Interaction/Components/GrabbableComponent.h"
#include "Net/UnrealNetwork.h"

ANPBaseRelic::ANPBaseRelic()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	RelicMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RelicMesh"));
	SetRootComponent(RelicMesh);
	RelicMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	RelicMesh->SetSimulatePhysics(false);

	GrabbableComponent = CreateDefaultSubobject<UGrabbableComponent>(TEXT("GrabbableComponent"));
}

void ANPBaseRelic::BeginPlay()
{
	Super::BeginPlay();

	GrabbableComponent->OnGrabStarted.AddUObject(
		this,
		&ANPBaseRelic::HandleGrabStarted);
}

void ANPBaseRelic::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPBaseRelic, bHasBeenInteracted);
}

void ANPBaseRelic::ActivatePhysics()
{
	if (!HasAuthority() || bHasBeenInteracted)
	{
		return;
	}

	bHasBeenInteracted = true;
	RelicMesh->SetSimulatePhysics(true);
	ForceNetUpdate();
}

void ANPBaseRelic::HandleGrabStarted(UPrimitiveComponent* GrabbedComponent)
{
	if (GrabbedComponent == RelicMesh)
	{
		ActivatePhysics();
	}
}
