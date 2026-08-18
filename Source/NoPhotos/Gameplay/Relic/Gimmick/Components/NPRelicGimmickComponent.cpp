#include "Gameplay/Relic/Gimmick/Components/NPRelicGimmickComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UNPRelicGimmickComponent::UNPRelicGimmickComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNPRelicGimmickComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNPRelicGimmickComponent, bIsCompleted);
}

void UNPRelicGimmickComponent::CompleteGimmick()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bIsCompleted)
	{
		return;
	}

	bIsCompleted = true;
	OnCompleted.Broadcast();
	GetOwner()->ForceNetUpdate();
}

void UNPRelicGimmickComponent::OnRep_IsCompleted()
{
	if (bIsCompleted)
	{
		OnCompleted.Broadcast();
	}
}
