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

	OnRep_IsDisplayed();
	GrabbableComponent->OnGrabStarted.AddUObject(
		this,
		&ANPBaseRelic::HandleGrabStarted);
}

void ANPBaseRelic::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPBaseRelic, bIsDisplayed);
	DOREPLIFETIME(ANPBaseRelic, bIsUnlocked);
}

void ANPBaseRelic::SetUnlocked(bool bUnlocked)
{
	if (!HasAuthority() || bIsUnlocked == bUnlocked)
	{
		return;
	}

	bIsUnlocked = bUnlocked;
	if (bIsUnlocked && GrabbableComponent->IsGrabbed())
	{
		ReleaseFromDisplay();
	}
	ForceNetUpdate();
}

void ANPBaseRelic::OnRep_IsDisplayed()
{
	const ECollisionEnabled::Type CollisionEnabled = RelicMesh->GetCollisionEnabled();
	const bool bHasPhysicsCollision =
		CollisionEnabled == ECollisionEnabled::QueryAndPhysics
		|| CollisionEnabled == ECollisionEnabled::PhysicsOnly;
	const bool bCanSimulate = RelicMesh->GetStaticMesh() && bHasPhysicsCollision;
	RelicMesh->SetSimulatePhysics(!bIsDisplayed && bCanSimulate);
}

void ANPBaseRelic::ReleaseFromDisplay()
{
	if (!HasAuthority() || !bIsDisplayed)
	{
		return;
	}

	bIsDisplayed = false;
	OnRep_IsDisplayed();
	ForceNetUpdate();
}

void ANPBaseRelic::HandleGrabStarted(UPrimitiveComponent*)
{
	if (bIsUnlocked)
	{
		ReleaseFromDisplay();
	}
}
