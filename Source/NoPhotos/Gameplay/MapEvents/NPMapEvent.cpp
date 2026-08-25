#include "NPMapEvent.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "NPMapEventDefinition.h"
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
	DOREPLIFETIME(ANPMapEvent, EventDefinition);
	DOREPLIFETIME(ANPMapEvent, bIsActive);
}

void ANPMapEvent::InitializeEvent(
	UNPMapEventDefinition* InEventDefinition,
	const float InSelectionWeight)
{
	if (!HasAuthority() || bIsActive)
	{
		return;
	}

	EventDefinition = InEventDefinition;
	RuntimeSelectionWeight = FMath::Max(0.0f, InSelectionWeight);
}

FName ANPMapEvent::GetEventId() const
{
	return EventDefinition ? EventDefinition->GetEventId() : EventId;
}

FText ANPMapEvent::GetEventDisplayName() const
{
	return EventDefinition ? EventDefinition->GetDisplayName() : EventDisplayName;
}

ENPMapEventType ANPMapEvent::GetEventType() const
{
	return EventDefinition ? EventDefinition->GetEventType() : EventType;
}

ENPMapEventScale ANPMapEvent::GetEventScale() const
{
	return EventDefinition ? EventDefinition->GetEventScale() : EventScale;
}

float ANPMapEvent::GetEventDuration() const
{
	return EventDefinition ? EventDefinition->GetDuration() : FMath::Max(0.0f, Duration);
}

bool ANPMapEvent::CanRunAtEventTime(const ENPMapEventType EventTimeType) const
{
	const ENPMapEventType ConfiguredType = GetEventType();
	return ConfiguredType == ENPMapEventType::Both || ConfiguredType == EventTimeType;
}

bool ANPMapEvent::StartEvent()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World || !CanStartEvent())
	{
		return false;
	}

	bIsActive = true;
	ApplyEventState(true);
	OnEventStarted.Broadcast(this);
	ForceNetUpdate();

	const float EventDuration = GetEventDuration();
	if (EventDuration > 0.0f)
	{
		World->GetTimerManager().SetTimer(DurationTimer, this, &ANPMapEvent::FinishEvent, EventDuration, false);
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
	ApplyEventState(false);
	OnEventFinished.Broadcast(this);
	ForceNetUpdate();
}

bool ANPMapEvent::CanStartEvent() const
{
	return HasAuthority()
		&& !bIsActive
		&& RuntimeSelectionWeight > 0.0f;
}

void ANPMapEvent::ApplyEventState_Implementation(const bool bNewActive)
{
}

void ANPMapEvent::OnRep_IsActive()
{
	ApplyEventState(bIsActive);
	if (bIsActive)
	{
		OnEventStarted.Broadcast(this);
	}
	else
	{
		OnEventFinished.Broadcast(this);
	}
}
