#include "NPMapEvent.h"

#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ANPMapEvent::ANPMapEvent()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);
}

void ANPMapEvent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANPMapEvent, bIsActive);
}

bool ANPMapEvent::StartEvent()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World || !CanStartEvent(World->GetTimeSeconds()))
	{
		return false;
	}

	bIsActive = true;
	ApplyEventState(true);
	ForceNetUpdate();

	if (Duration > 0.0f)
	{
		World->GetTimerManager().SetTimer(DurationTimer, this, &ANPMapEvent::FinishEvent, Duration, false);
	}

	return true;
}

void ANPMapEvent::FinishEvent()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World || !bIsActive)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(DurationTimer);
	bIsActive = false;
	LastFinishedServerTime = World->GetTimeSeconds();
	ApplyEventState(false);
	ForceNetUpdate();
}

bool ANPMapEvent::CanStartEvent(const double ServerTimeSeconds) const
{
	return HasAuthority()
		&& !bIsActive
		&& SelectionWeight > 0.0f
		&& (LastFinishedServerTime < 0.0 || ServerTimeSeconds >= LastFinishedServerTime + Cooldown);
}

void ANPMapEvent::ApplyEventState_Implementation(const bool bNewActive)
{
}

void ANPMapEvent::OnRep_IsActive()
{
	ApplyEventState(bIsActive);
}
