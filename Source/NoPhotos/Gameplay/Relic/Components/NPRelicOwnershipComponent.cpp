#include "Gameplay/Relic/Components/NPRelicOwnershipComponent.h"

#include "Core/NPPlayerState.h"
#include "GameFramework/Actor.h"
#include "Gameplay/Character/Component/NPStablePhysicsGrabComponent.h"

UNPRelicOwnershipComponent::UNPRelicOwnershipComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

bool UNPRelicOwnershipComponent::HasServerAuthority() const
{
	const AActor* OwnerActor = GetOwner();
	return IsValid(OwnerActor) && OwnerActor->HasAuthority();
}

void UNPRelicOwnershipComponent::RegisterGrabber(
	UNPStablePhysicsGrabComponent* GrabComponent,
	ANPPlayerState* PlayerState)
{
	if (!HasServerAuthority() || !IsValid(GrabComponent) || !IsValid(PlayerState))
	{
		return;
	}

	ActiveGrabbers.FindOrAdd(GrabComponent) = PlayerState;
}

void UNPRelicOwnershipComponent::UnregisterGrabber(
	UNPStablePhysicsGrabComponent* GrabComponent)
{
	if (!HasServerAuthority() || !IsValid(GrabComponent))
	{
		return;
	}

	ActiveGrabbers.Remove(GrabComponent);
}

void UNPRelicOwnershipComponent::GetCurrentOwners(
	TArray<ANPPlayerState*>& OutOwners) const
{
	OutOwners.Reset();
	TSet<ANPPlayerState*> UniqueOwners;

	for (const TPair<
		TWeakObjectPtr<UNPStablePhysicsGrabComponent>,
		TWeakObjectPtr<ANPPlayerState>>& Pair : ActiveGrabbers)
	{
		if (Pair.Key.IsValid() && Pair.Value.IsValid())
		{
			UniqueOwners.Add(Pair.Value.Get());
		}
	}

	OutOwners.Reserve(UniqueOwners.Num());
	for (ANPPlayerState* Owner : UniqueOwners)
	{
		OutOwners.Add(Owner);
	}
}

int32 UNPRelicOwnershipComponent::GetCurrentOwnerCount() const
{
	TArray<ANPPlayerState*> Owners;
	GetCurrentOwners(Owners);
	return Owners.Num();
}

void UNPRelicOwnershipComponent::ReleaseAllGrabbers()
{
	if (!HasServerAuthority())
	{
		return;
	}

	TArray<TWeakObjectPtr<UNPStablePhysicsGrabComponent>> Grabbers;
	ActiveGrabbers.GenerateKeyArray(Grabbers);
	for (const TWeakObjectPtr<UNPStablePhysicsGrabComponent>& Grabber : Grabbers)
	{
		if (Grabber.IsValid())
		{
			Grabber->SetGrabRequested(false);
		}
	}
}

void UNPRelicOwnershipComponent::ClearOwnership()
{
	if (HasServerAuthority())
	{
		ActiveGrabbers.Reset();
	}
}
