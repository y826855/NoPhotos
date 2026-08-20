#include "Gameplay/Photo/NPPhotoFlashWidget.h"

#include "Engine/World.h"
#include "TimerManager.h"

void UNPPhotoFlashWidget::PlayFlash()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(FlashHideTimer);
	SetRenderOpacity(1.0f);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	BP_PlayFlashAnimation();

	if (FlashDisplayDuration <= 0.0f)
	{
		FinishFlash();
		return;
	}

	World->GetTimerManager().SetTimer(
		FlashHideTimer,
		this,
		&UNPPhotoFlashWidget::FinishFlash,
		FlashDisplayDuration,
		false);
}

void UNPPhotoFlashWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FlashHideTimer);
	}

	Super::NativeDestruct();
}

void UNPPhotoFlashWidget::FinishFlash()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
