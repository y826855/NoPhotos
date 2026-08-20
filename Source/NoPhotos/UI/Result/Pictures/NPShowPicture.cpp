#include "UI/Result/Pictures/NPShowPicture.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UNPShowPicture::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(SelectButton))
	{
		SelectButton->OnClicked.AddDynamic(
			this,
			&UNPShowPicture::OnSelectButtonClicked);
	}

	SetSelected(false);
}

void UNPShowPicture::SetPicture(
	UTexture2D* InTexture,
	const int32 InPictureIndex)
{
	if (!IsValid(SelectPicture) || !IsValid(InTexture))
	{
		return;
	}

	CurrentPictureIndex = InPictureIndex;
	SelectPicture->SetBrushFromTexture(InTexture);
}

void UNPShowPicture::SetSelected(const bool InSelected)
{
	IsSelected = InSelected;
}

void UNPShowPicture::OnSelectButtonClicked()
{
	if (CurrentPictureIndex == INDEX_NONE)
	{
		return;
	}

	OnSelectRequested.Broadcast(CurrentPictureIndex);
}