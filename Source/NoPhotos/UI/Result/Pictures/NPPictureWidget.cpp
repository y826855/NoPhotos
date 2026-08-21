#include "UI/Result/Pictures/NPPictureWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UNPPictureWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Button))
	{
		Button->OnClicked.AddDynamic(this, &UNPPictureWidget::HandleButtonClicked);
	}
	
	SetSelected(false);
}

void UNPPictureWidget::HandleButtonClicked()
{
	OnPictureClicked.Broadcast(this);
}

void UNPPictureWidget::SetPicture(UTexture2D* InTexture)
{
	if (!IsValid(Picture) || !IsValid(InTexture))
	{
		return;
	}

	Picture->SetBrushFromTexture(InTexture);
}

void UNPPictureWidget::SetSelected(const bool bInSelected)
{
	IsSelected = bInSelected;

	if (!IsValid(SelectMark))
	{
		return;
	}

	SelectMark->SetVisibility(
		IsSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}