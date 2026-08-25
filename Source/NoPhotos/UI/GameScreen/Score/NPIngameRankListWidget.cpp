#include "UI/GameScreen/Score/NPIngameRankListWidget.h"

#include "Components/VerticalBox.h"
#include "Core/Main/NPMainGameState.h"
#include "Core/NPPlayerState.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UI/GameScreen/Score/NPIngameRankObjectWidget.h"

void UNPIngameRankListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RankListInitializationTimer,
			this,
			&ThisClass::TryInitializeRankList,
			0.1f,
			true);
	}

	TryInitializeRankList();
}

void UNPIngameRankListWidget::NativeDestruct()
{
	if (IsValid(MainGameState))
	{
		MainGameState->OnPlayerRankingsChanged.RemoveAll(this);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RankListInitializationTimer);
	}
	MainGameState = nullptr;

	Super::NativeDestruct();
}

void UNPIngameRankListWidget::TryInitializeRankList()
{
	if (!TryBindToMainGameState())
	{
		return;
	}

	SetRankList();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RankListInitializationTimer);
	}
}

void UNPIngameRankListWidget::HandlePlayerRankingsChanged()
{
	SetRankList();
}

bool UNPIngameRankListWidget::TryBindToMainGameState()
{
	if (!IsValid(MainGameState))
	{
		MainGameState = GetWorld() ? GetWorld()->GetGameState<ANPMainGameState>() : nullptr;
	}

	if (!IsValid(MainGameState))
	{
		return false;
	}

	MainGameState->OnPlayerRankingsChanged.AddUniqueDynamic(this, &ThisClass::HandlePlayerRankingsChanged);
	return true;
}

void UNPIngameRankListWidget::SetRankList()
{
	if (!IsValid(RankList) || !IsValid(RankObjectWidgetClass))
	{
		return;
	}

	if (!TryBindToMainGameState())
	{
		return;
	}

	RankList->ClearChildren();

	const TArray<FNPPlayerRanking> PlayerRankings =	MainGameState->GetPlayerRankings();

	for (int32 RankingIndex = 0;RankingIndex < PlayerRankings.Num();++RankingIndex)
	{
		const FNPPlayerRanking& Ranking = PlayerRankings[RankingIndex];
		const FString PlayerName = IsValid(Ranking.PlayerState) ? Ranking.PlayerState->GetPlayerName() : TEXT("Unknown");

		UNPIngameRankObjectWidget* RankObjectWidget = CreateWidget<UNPIngameRankObjectWidget>(GetOwningPlayer(), RankObjectWidgetClass);

		if (!IsValid(RankObjectWidget))
		{
			continue;
		}

		RankObjectWidget->SetPersonalScore(RankingIndex + 1, PlayerName, Ranking.Score);
		RankList->AddChild(RankObjectWidget);
	}
}
