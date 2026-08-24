#include "Gameplay/Relic/NPBaseRelic.h"

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/Structs/NPRelicData.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/Interaction/Components/GrabbableComponent.h"
#include "Net/UnrealNetwork.h"

const FName ANPBaseRelic::RelicComponentName(TEXT("RelicMesh"));

ANPBaseRelic::ANPBaseRelic(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	RelicMesh = CreateDefaultSubobject<
		UPrimitiveComponent,
		UStaticMeshComponent>(RelicComponentName);
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
	const ECollisionEnabled::Type CollisionEnabled = RelicMesh->GetCollisionEnabled();

	const bool bHasPhysicsCollision =
		CollisionEnabled == ECollisionEnabled::QueryAndPhysics
		|| CollisionEnabled == ECollisionEnabled::PhysicsOnly;

	const bool bCanSimulate = RelicMesh->CanEditSimulatePhysics()
		&& bHasPhysicsCollision;
	
	RelicMesh->SetSimulatePhysics(!bIsDisplayed && !bIsReturned && bCanSimulate);
}

void ANPBaseRelic::SetLastCarrierPlayerState(APlayerState* PlayerState)
{
	if (!HasAuthority() || bIsReturned || !IsValid(PlayerState)
		|| LastCarrierPlayerState == PlayerState)
	{
		return;
	}

	LastCarrierPlayerState = PlayerState;
}

bool ANPBaseRelic::RegisterEvidencePhotographer(APlayerState* Photographer)
{
	if (!HasAuthority() || bIsReturned || !IsValid(Photographer)
		|| EvidencePhotographers.Contains(Photographer)
		|| EvidencePhotographers.Num() >= MaximumEvidencePhotographers)
	{
		return false;
	}

	EvidencePhotographers.Add(Photographer);
	return true;
}

bool ANPBaseRelic::TryMarkReturned()
{
	if (!HasAuthority() || bIsReturned)
	{
		return false;
	}

	FlushNetDormancy();
	bIsReturned = true;
	SetReplicateMovement(false);
	OnRep_IsReturned();
	ForceNetUpdate();
	SetNetDormancy(DORM_DormantAll);
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
