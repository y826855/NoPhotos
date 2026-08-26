#include "Gameplay/Relic/Gimmick/Components/NPGimmickProgressComponent.h"

UNPGimmickProgressComponent::UNPGimmickProgressComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UNPGimmickProgressComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float PreviousProgress = CurrentProgress;
	const float Duration = ProgressDirection > 0.0f
		? ProgressDuration
		: ReverseDuration;
	CurrentProgress = FMath::Clamp(
		CurrentProgress + ProgressDirection * DeltaTime / FMath::Max(Duration, UE_SMALL_NUMBER),
		0.0f,
		1.0f);

	if (!FMath::IsNearlyEqual(PreviousProgress, CurrentProgress))
	{
		OnProgressChanged.Broadcast(CurrentProgress);
	}

	if (PreviousProgress < 1.0f && CurrentProgress >= 1.0f)
	{
		ProgressDirection = 0.0f;
		SetComponentTickEnabled(false);
		OnProgressCompleted.Broadcast();
	}
	else if (PreviousProgress >= 1.0f && CurrentProgress < 1.0f)
	{
		OnProgressLost.Broadcast();
	}

	if (CurrentProgress <= 0.0f && ProgressDirection < 0.0f)
	{
		ProgressDirection = 0.0f;
		SetComponentTickEnabled(false);
	}
}

void UNPGimmickProgressComponent::StartProgress()
{
	if (CurrentProgress >= 1.0f)
	{
		return;
	}

	ProgressDirection = 1.0f;
	SetComponentTickEnabled(true);
}

void UNPGimmickProgressComponent::ReverseProgress()
{
	if (CurrentProgress <= 0.0f)
	{
		return;
	}

	ProgressDirection = -1.0f;
	SetComponentTickEnabled(true);
}
