#include "UI/GameScreen/NPTimerWidget.h"
#include "Components/TextBlock.h"
#include "Core/Main/NPMainGameState.h"
#include "Engine/World.h"
#include "TimerManager.h"

UNPTimerWidget::UNPTimerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNPTimerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TryBindToMainGameState())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			GameStateBindRetryTimerHandle,
			this,
			&UNPTimerWidget::RetryBindToMainGameState,
			0.1f,
			true);
	}

	UpdateTimerUI(0);
}

void UNPTimerWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GameStateBindRetryTimerHandle);
	}

	UnbindFromMainGameState();

	Super::NativeDestruct();
}

void UNPTimerWidget::OnMainGameStateChanged()
{
	if (UWorld* World = GetWorld())
	{
		if (ANPMainGameState* MainGameState =
			World->GetGameState<ANPMainGameState>())
		{
			UpdateTimerUI(MainGameState->GetRemainingGameTime());
		}
	}
}

bool UNPTimerWidget::TryBindToMainGameState()
{
	UWorld* World = GetWorld();
	ANPMainGameState* MainGameState = World
		? World->GetGameState<ANPMainGameState>()
		: nullptr;

	if (!IsValid(MainGameState))
	{
		return false;
	}

	if (BoundMainGameState.Get() != MainGameState)
	{
		UnbindFromMainGameState();

		BoundMainGameState = MainGameState;
		MainGameState->OnMainGameStateChanged.AddUniqueDynamic(
			this,
			&UNPTimerWidget::OnMainGameStateChanged);
	}

	UpdateTimerUI(MainGameState->GetRemainingGameTime());
	return true;
}

void UNPTimerWidget::RetryBindToMainGameState()
{
	if (!TryBindToMainGameState())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GameStateBindRetryTimerHandle);
	}
}

void UNPTimerWidget::UnbindFromMainGameState()
{
	if (ANPMainGameState* MainGameState = BoundMainGameState.Get())
	{
		MainGameState->OnMainGameStateChanged.RemoveAll(this);
	}

	BoundMainGameState.Reset();
}

void UNPTimerWidget::UpdateTimerUI(int32 RemainingTimeSeconds)
{
	const int32 ClampedRemainingTime = FMath::Max(0, RemainingTimeSeconds);
	const int32 Minutes = ClampedRemainingTime / 60;
	const int32 Seconds = ClampedRemainingTime % 60;

	if (IsValid(MinuteText))
	{
		MinuteText->SetText(
			FText::FromString(FString::Printf(TEXT("%02d"), Minutes)));
	}

	if (IsValid(SecondText))
	{
		SecondText->SetText(
			FText::FromString(FString::Printf(TEXT("%02d"), Seconds)));
	}
}