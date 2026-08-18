#include "Gameplay/Photo/NPPhotoPreviewWidget.h"

#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Gameplay/Photo/NPPhotoLog.h"

void UNPPhotoPreviewWidget::ShowPhoto(UTextureRenderTarget2D* InPhoto)
{
	if (!InPhoto)
	{
		UE_LOG(LogNPPhoto, Error, TEXT("[PhotoUI] Cannot display a null Render Target."));
		return;
	}

	DisplayedPhoto = InPhoto;
	if (PhotoImage)
	{
		PhotoImage->SetBrushResourceObject(InPhoto);
	}
	else
	{
		UE_LOG(
			LogNPPhoto,
			Warning,
			TEXT("[PhotoUI] PhotoImage is not bound. Add an Image named PhotoImage to the WBP."));
	}

	SetVisibility(ESlateVisibility::Visible);
	BP_OnPhotoDisplayed(InPhoto);
	UE_LOG(LogNPPhoto, Log, TEXT("[PhotoUI] Photo displayed. Target=%s"), *GetNameSafe(InPhoto));
}

void UNPPhotoPreviewWidget::HidePhoto()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
