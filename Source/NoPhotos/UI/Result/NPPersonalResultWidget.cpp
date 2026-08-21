#include "UI/Result/NPPersonalResultWidget.h"
#include "Components/TextBlock.h"

void UNPPersonalResultWidget::SetupResult(const int32 InRank, const FString& InPlayerName, const int32 InScore)
{
	if (IsValid(RankText))
	{
		RankText->SetText(FText::AsNumber(InRank));
	}

	if (IsValid(PlayerNameText))
	{
		PlayerNameText->SetText(FText::FromString(InPlayerName));
	}

	if (IsValid(ScoreText))
	{
		ScoreText->SetText(FText::AsNumber(InScore));
	}
}