#include "Gameplay/Photo/NPPhotoPreviewWidget.h"

#include "Components/Image.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Gameplay/Photo/NPPhotoLog.h"
#include "TimerManager.h"

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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreviewHideTimer);
		if (PreviewDuration > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				PreviewHideTimer,
				this,
				&UNPPhotoPreviewWidget::HidePhoto,
				PreviewDuration,
				false);
		}
	}

	UE_LOG(
		LogNPPhoto,
		Log,
		TEXT("[PhotoUI] Photo displayed. Target=%s Duration=%.2f"),
		*GetNameSafe(InPhoto),
		PreviewDuration);
}

void UNPPhotoPreviewWidget::HidePhoto()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreviewHideTimer);
	}
	SetVisibility(ESlateVisibility::Collapsed);
	UE_LOG(LogNPPhoto, Log, TEXT("[PhotoUI] Photo preview hidden."));
}

void UNPPhotoPreviewWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreviewHideTimer);
	}
	Super::NativeDestruct();
}
