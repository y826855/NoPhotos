#include "Gameplay/Relic/NPBaseRelic.h"

#include "Components/StaticMeshComponent.h"
#include "Data/Structs/NPRelicData.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/PlayerState.h"
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
	OnRep_IsReturned();
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
	DOREPLIFETIME(ANPBaseRelic, bIsReturned);
	DOREPLIFETIME(ANPBaseRelic, LastCarrierPlayerState);
	DOREPLIFETIME(ANPBaseRelic, EvidencePhotographers);
}

int32 ANPBaseRelic::GetBasePrice() const
{
	const FNPRelicData* Data = RelicData.GetRow<FNPRelicData>(TEXT("GetBasePrice"));
	return Data ? FMath::Max(0, Data->Price) : 0;
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
	RelicMesh->SetSimulatePhysics(!bIsDisplayed && !bIsReturned);
}

void ANPBaseRelic::SetLastCarrierPlayerState(APlayerState* PlayerState)
{
	if (!HasAuthority() || bIsReturned || !IsValid(PlayerState))
	{
		return;
	}

	LastCarrierPlayerState = PlayerState;
	ForceNetUpdate();
}

bool ANPBaseRelic::RegisterEvidencePhotographer(APlayerState* Photographer)
{
	if (!HasAuthority() || bIsReturned || !IsValid(Photographer)
		|| EvidencePhotographers.Contains(Photographer))
	{
		return false;
	}

	EvidencePhotographers.Add(Photographer);
	ForceNetUpdate();
	return true;
}

bool ANPBaseRelic::TryMarkReturned()
{
	if (!HasAuthority() || bIsReturned)
	{
		return false;
	}

	bIsReturned = true;
	OnRep_IsReturned();
	ForceNetUpdate();
	return true;
}

void ANPBaseRelic::OnRep_IsReturned()
{
	if (!bIsReturned)
	{
		return;
	}

	GrabbableComponent->SetGrabEnabled(false);
	RelicMesh->SetSimulatePhysics(false);
	RelicMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RelicMesh->SetVisibility(false, true);
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
