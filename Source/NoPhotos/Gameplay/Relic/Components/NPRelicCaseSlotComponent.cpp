#include "Gameplay/Relic/Components/NPRelicCaseSlotComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Gameplay/Relic/NPBaseRelic.h"
#include "Net/UnrealNetwork.h"

UNPRelicCaseSlotComponent::UNPRelicCaseSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNPRelicCaseSlotComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNPRelicCaseSlotComponent, SpawnedRelic);
	DOREPLIFETIME(UNPRelicCaseSlotComponent, bIsRelicReleased);
}

ANPBaseRelic* UNPRelicCaseSlotComponent::SpawnRelic(
	const bool bInitiallyReleased)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || IsValid(SpawnedRelic) ||
		!RelicClass)
	{
		return SpawnedRelic;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	bIsRelicReleased = bInitiallyReleased;
	SpawnedRelic = World->SpawnActor<ANPBaseRelic>(
		RelicClass,
		GetComponentTransform(),
		SpawnParameters);
	if (!IsValid(SpawnedRelic))
	{
		return nullptr;
	}

	SpawnedRelic->SetUnlocked(bIsRelicReleased);
	ApplyRelicState();
	Owner->ForceNetUpdate();
	return SpawnedRelic;
}

void UNPRelicCaseSlotComponent::ReleaseRelic()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || bIsRelicReleased)
	{
		return;
	}

	bIsRelicReleased = true;
	if (IsValid(SpawnedRelic))
	{
		SpawnedRelic->SetUnlocked(true);
	}
	ApplyRelicState();
	Owner->ForceNetUpdate();
}

void UNPRelicCaseSlotComponent::OnRep_SpawnedRelic()
{
	ApplyRelicState();
}

void UNPRelicCaseSlotComponent::OnRep_IsRelicReleased()
{
	ApplyRelicState();
}

void UNPRelicCaseSlotComponent::ApplyRelicState()
{
	if (!IsValid(SpawnedRelic))
	{
		return;
	}

	if (UPrimitiveComponent* RelicPrimitive =
		Cast<UPrimitiveComponent>(SpawnedRelic->GetRootComponent()))
	{
		RelicPrimitive->SetCollisionResponseToChannel(
			ECC_Destructible,
			ECR_Ignore);
	}
	SpawnedRelic->SetActorEnableCollision(bIsRelicReleased);
}
