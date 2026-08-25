#include "UI/GameScreen/Score/NPIngameRankObjectWidget.h"
#include "Components/TextBlock.h"

void UNPIngameRankObjectWidget::SetPersonalScore(const int32 InRank, const FString& InPlayerName, const int32 InScore)
{
	if (!IsValid(PersonalScore))
	{
		return;
	}

	PersonalScore->SetText(FText::FromString(FString::Printf(TEXT("%d. %s : %d점"), InRank,*InPlayerName,InScore)));
}

