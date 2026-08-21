#include "UI/Result/NPPersonalResultWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"
#include "UI/Result/Pictures/NPResultPicturePopup.h"

void UNPPersonalResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(ShowPictureButton))
	{
		ShowPictureButton->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleShowPictureButtonClicked);
	}
}

void UNPPersonalResultWidget::NativeDestruct()
{
	if (IsValid(ShowPictureButton))
	{
		ShowPictureButton->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UNPPersonalResultWidget::SetupResult(const int32 InRank, const FString& InPlayerName, const int32 InScore, APlayerState* InPlayerState)
{
	ResultPlayerState = InPlayerState;

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

	if (IsValid(ShowPictureButton))
	{
		ShowPictureButton->SetIsEnabled(IsValid(ResultPlayerState));
	}
}

void UNPPersonalResultWidget::HandleShowPictureButtonClicked()
{
	if (!IsValid(ResultPlayerState))
	{
		return;
	}

	if (!IsValid(PicturePopupWidgetClass))
	{
		return;
	}

	UNPResultPicturePopup* Popup = CreateWidget<UNPResultPicturePopup>(
		GetOwningPlayer(),
		PicturePopupWidgetClass);
	if (!IsValid(Popup))
	{
		return;
	}

	Popup->AddToViewport(100);
	Popup->OpenForPlayer(ResultPlayerState);
}
