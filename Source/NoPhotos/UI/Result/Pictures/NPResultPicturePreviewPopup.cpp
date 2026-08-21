#include "UI/Result/Pictures/NPResultPicturePreviewPopup.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UNPResultPicturePreviewPopup::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}
}

void UNPResultPicturePreviewPopup::OpenWithTexture(UTexture2D* InTexture)
{
	if (IsValid(PreviewImage) && IsValid(InTexture))
	{
		PreviewImage->SetBrushFromTexture(InTexture);
	}
}

void UNPResultPicturePreviewPopup::HandleCloseClicked()
{
	RemoveFromParent();
}
