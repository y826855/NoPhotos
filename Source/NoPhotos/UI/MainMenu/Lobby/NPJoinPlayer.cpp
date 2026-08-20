#include "UI/MainMenu/Lobby/NPJoinPlayer.h"
#include "Components/TextBlock.h"

void UNPJoinPlayer::SetupResult(int32 InNumber, const FString& InPlayerName)
{
	if (IsValid(JoinNumberText))
	{
		JoinNumberText->SetText(FText::AsNumber(InNumber));
	}

	if (IsValid(PlayerNameText))
	{
		PlayerNameText->SetText(FText::FromString(InPlayerName));
	}
}
