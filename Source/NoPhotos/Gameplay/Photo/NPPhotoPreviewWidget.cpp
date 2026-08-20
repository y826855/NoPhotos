#include "Gameplay/Photo/NPPhotoPreviewWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Gameplay/Photo/NPPhotoLog.h"
#include "Gameplay/Photo/NPPhotoTransferComponent.h"
#include "NoPhotosGameState.h"
#include "NoPhotosPlayerController.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"

void UNPPhotoPreviewWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (PhotoImage)
	{
		PhotoImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PhotoTransferText)
	{
		PhotoTransferText->SetText(FText::FromString(TEXT("사진 전송")));
	}
	if (PhotoReceiveButton)
	{
		PhotoReceiveButton->OnClicked.AddUniqueDynamic(this, &UNPPhotoPreviewWidget::HandleReceiveButtonClicked);
	}

	if (ANoPhotosPlayerController* Controller = Cast<ANoPhotosPlayerController>(GetOwningPlayer()))
	{
		TransferComponent = Controller->GetPhotoTransferComponent();
		if (TransferComponent)
		{
			TransferComponent->OnPhotoTextureReceived.AddUniqueDynamic(this, &UNPPhotoPreviewWidget::HandlePhotoTextureReceived);
		}
	}

	ObservedGameState = GetWorld() ? GetWorld()->GetGameState<ANoPhotosGameState>() : nullptr;
	if (ObservedGameState)
	{
		ObservedGameState->OnPhotoEvidenceChanged.AddUniqueDynamic(this, &UNPPhotoPreviewWidget::HandlePhotoEvidenceChanged);
	}

	SetTransferNotificationVisible(false);
	RefreshAvailablePhotos();
}

void UNPPhotoPreviewWidget::ShowPhoto(UTexture* InPhoto)
{
	if (!IsValid(InPhoto))
	{
		UE_LOG(LogNPPhoto, Error, TEXT("[PhotoUI] Cannot display a null texture."));
		return;
	}

	DisplayedPhoto = InPhoto;
	if (PhotoImage)
	{
		PhotoImage->SetBrushResourceObject(InPhoto);
		PhotoImage->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		UE_LOG(LogNPPhoto, Warning, TEXT("[PhotoUI] PhotoImage is not bound."));
	}

	BP_OnPhotoDisplayed(InPhoto);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreviewHideTimer);
		if (PreviewDuration > 0.0f)
		{
			World->GetTimerManager().SetTimer(PreviewHideTimer, this, &UNPPhotoPreviewWidget::HidePhoto, PreviewDuration, false);
		}
	}
	UE_LOG(LogNPPhoto, Log, TEXT("[PhotoUI] Received photo displayed for %.2f seconds."), PreviewDuration);
}

void UNPPhotoPreviewWidget::HidePhoto()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreviewHideTimer);
	}
	if (PhotoImage)
	{
		PhotoImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	DisplayedPhoto = nullptr;
}

void UNPPhotoPreviewWidget::HandlePhotoEvidenceChanged()
{
	RefreshAvailablePhotos();
}

void UNPPhotoPreviewWidget::RefreshAvailablePhotos()
{
	if (!ObservedGameState)
	{
		ObservedGameState = GetWorld() ? GetWorld()->GetGameState<ANoPhotosGameState>() : nullptr;
	}
	if (!ObservedGameState)
	{
		return;
	}

	for (const FGuid& PhotoId : ObservedGameState->GetTransferredPhotoIds())
	{
		if (PhotoId.IsValid() && !KnownPhotoIds.Contains(PhotoId))
		{
			KnownPhotoIds.Add(PhotoId);
			AvailablePhotoIds.Add(PhotoId);
			UE_LOG(LogNPPhoto, Log, TEXT("[PhotoUI] New transferred photo available. PhotoId=%s"), *PhotoId.ToString());
		}
	}
	SetTransferNotificationVisible(!AvailablePhotoIds.IsEmpty() || PendingPhotoId.IsValid());
}

void UNPPhotoPreviewWidget::HandleReceiveButtonClicked()
{
	if (!TransferComponent || PendingPhotoId.IsValid() || AvailablePhotoIds.IsEmpty())
	{
		return;
	}

	PendingPhotoId = AvailablePhotoIds[0];
	AvailablePhotoIds.RemoveAt(0);
	if (PhotoReceiveButton)
	{
		PhotoReceiveButton->SetIsEnabled(false);
	}

	if (UTexture2D* CachedTexture = TransferComponent->FindReceivedPhoto(PendingPhotoId))
	{
		HandlePhotoTextureReceived(PendingPhotoId, CachedTexture);
		return;
	}

	TransferComponent->RequestPhoto(PendingPhotoId);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TransferRequestTimer, this, &UNPPhotoPreviewWidget::HandleTransferRequestTimeout, TransferRequestTimeout, false);
	}
	UE_LOG(LogNPPhoto, Log, TEXT("[PhotoUI] Photo download requested. PhotoId=%s"), *PendingPhotoId.ToString());
}

void UNPPhotoPreviewWidget::HandlePhotoTextureReceived(const FGuid PhotoId, UTexture2D* Texture)
{
	if (PhotoId != PendingPhotoId || !IsValid(Texture))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TransferRequestTimer);
	}
	PendingPhotoId.Invalidate();
	if (PhotoReceiveButton)
	{
		PhotoReceiveButton->SetIsEnabled(true);
	}
	SetTransferNotificationVisible(!AvailablePhotoIds.IsEmpty());
	ShowPhoto(Texture);
}

void UNPPhotoPreviewWidget::SetTransferNotificationVisible(const bool bVisible)
{
	const ESlateVisibility NotificationVisibility = bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;

	if (PhotoReceiveButton)
	{
		PhotoReceiveButton->SetVisibility(NotificationVisibility);
	}
	if (PhotoTransferText)
	{
		PhotoTransferText->SetVisibility(NotificationVisibility);
	}

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		if (bVisible)
		{
			PlayerController->bShowMouseCursor = true;

			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(TakeWidget());
			InputMode.SetHideCursorDuringCapture(false);
			InputMode.SetLockMouseToViewportBehavior(
				EMouseLockMode::DoNotLock);

			PlayerController->SetInputMode(InputMode);
		}
		else
		{
			PlayerController->bShowMouseCursor = false;
			PlayerController->SetInputMode(FInputModeGameOnly());
		}
	}
}

void UNPPhotoPreviewWidget::HandleTransferRequestTimeout()
{
	if (PendingPhotoId.IsValid())
	{
		AvailablePhotoIds.Insert(PendingPhotoId, 0);
		PendingPhotoId.Invalidate();
	}
	if (PhotoReceiveButton)
	{
		PhotoReceiveButton->SetIsEnabled(true);
	}
	SetTransferNotificationVisible(!AvailablePhotoIds.IsEmpty());
	UE_LOG(LogNPPhoto, Warning, TEXT("[PhotoUI] Photo download timed out."));
}

void UNPPhotoPreviewWidget::NativeDestruct()
{
	if (PhotoReceiveButton)
	{
		PhotoReceiveButton->OnClicked.RemoveDynamic(this, &UNPPhotoPreviewWidget::HandleReceiveButtonClicked);
	}
	if (TransferComponent)
	{
		TransferComponent->OnPhotoTextureReceived.RemoveDynamic(this, &UNPPhotoPreviewWidget::HandlePhotoTextureReceived);
	}
	if (ObservedGameState)
	{
		ObservedGameState->OnPhotoEvidenceChanged.RemoveDynamic(this, &UNPPhotoPreviewWidget::HandlePhotoEvidenceChanged);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreviewHideTimer);
		World->GetTimerManager().ClearTimer(TransferRequestTimer);
	}
	Super::NativeDestruct();
}
