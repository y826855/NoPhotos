#include "UI/Result/NPResultListWidget.h"

#include "Components/VerticalBox.h"
#include "Core/NPPlayerState.h"
#include "Core/Main/NPMainGameState.h"
#include "UI/Result/NPPersonalResultWidget.h"

void UNPResultListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshResultList();
}

void UNPResultListWidget::RefreshResultList()
{
	if (!IsValid(RankList) || !IsValid(PersonalResultWidgetClass))
	{
		return;
	}

	ANPMainGameState* NPMGS = GetWorld()
		? GetWorld()->GetGameState<ANPMainGameState>()
		: nullptr;

	if (!IsValid(NPMGS))
	{
		return;
	}

	RankList->ClearChildren();

	const TArray<FNPPlayerRanking> PlayerRankings = NPMGS->GetPlayerRankings();

	for (int32 RankingIndex = 0; RankingIndex < PlayerRankings.Num(); ++RankingIndex)
	{
		const FNPPlayerRanking& Ranking = PlayerRankings[RankingIndex];
		const FString PlayerName = Ranking.PlayerState
			? Ranking.PlayerState->GetPlayerName()
			: TEXT("Unknown");

		UNPPersonalResultWidget* PersonalResultWidget =
			CreateWidget<UNPPersonalResultWidget>(GetOwningPlayer(), PersonalResultWidgetClass);

		if (!IsValid(PersonalResultWidget))
		{
			continue;
		}

		PersonalResultWidget->SetupResult(
			RankingIndex + 1,
			PlayerName,
			Ranking.Score);

		RankList->AddChild(PersonalResultWidget);
	}
}