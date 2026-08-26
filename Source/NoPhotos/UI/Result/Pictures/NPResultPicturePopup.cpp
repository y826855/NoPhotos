#include "UI/Result/Pictures/NPResultPicturePopup.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Core/Main/NPMainPlayerController.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/Photo/NPPhotoTransferComponent.h"
#include "Core/Main/NPMainGameState.h"
#include "UI/Result/Pictures/NPResultPicturePreviewPopup.h"

void UNPResultPicturePopup::NativeConstruct()
{
	Super::NativeConstruct();

	ThumbnailButtons = {
		ThumbnailButton0,
		ThumbnailButton1,
		ThumbnailButton2,
		ThumbnailButton3,
		ThumbnailButton4
	};

	ThumbnailImages = {
		ThumbnailImage0,
		ThumbnailImage1,
		ThumbnailImage2,
		ThumbnailImage3,
		ThumbnailImage4
	};

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}

	if (ThumbnailButtons.IsValidIndex(0) && IsValid(ThumbnailButtons[0]))
	{
		ThumbnailButtons[0]->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleThumbnail0Clicked);
	}
	if (ThumbnailButtons.IsValidIndex(1) && IsValid(ThumbnailButtons[1]))
	{
		ThumbnailButtons[1]->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleThumbnail1Clicked);
	}
	if (ThumbnailButtons.IsValidIndex(2) && IsValid(ThumbnailButtons[2]))
	{
		ThumbnailButtons[2]->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleThumbnail2Clicked);
	}
	if (ThumbnailButtons.IsValidIndex(3) && IsValid(ThumbnailButtons[3]))
	{
		ThumbnailButtons[3]->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleThumbnail3Clicked);
	}
	if (ThumbnailButtons.IsValidIndex(4) && IsValid(ThumbnailButtons[4]))
	{
		ThumbnailButtons[4]->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleThumbnail4Clicked);
	}

	for (UButton* ThumbnailButton : ThumbnailButtons)
	{
		if (IsValid(ThumbnailButton))
		{
			ThumbnailButton->SetIsEnabled(false);
		}
	}
}

void UNPResultPicturePopup::NativeDestruct()
{
	if (IsValid(TransferComponent))
	{
		TransferComponent->OnPhotoTextureReceived.RemoveDynamic(
			this,
			&ThisClass::HandlePhotoTextureReceived);
	}

	Super::NativeDestruct();
}

void UNPResultPicturePopup::OpenForPlayer(APlayerState* InPlayerState)
{
	ViewedPlayerState = InPlayerState;
	PictureTextures.Empty();
	PendingPhotoIds.Empty();
	DownloadingPhotoId.Invalidate();

	if (!IsValid(ViewedPlayerState))
	{
		UpdateStatusText(TEXT("유저 정보를 찾을 수 없습니다."));
		return;
	}

	UpdateStatusText(FString::Printf(
		TEXT("%s 님이 고른 사진"),
		*ViewedPlayerState->GetPlayerName()));

	ANPMainGameState* GameState = GetWorld()
		? GetWorld()->GetGameState<ANPMainGameState>()
		: nullptr;

	ANPMainPlayerController* PlayerController =
		Cast<ANPMainPlayerController>(GetOwningPlayer());
	TransferComponent = IsValid(PlayerController)
		? PlayerController->GetPhotoTransferComponent()
		: nullptr;

	if (!IsValid(GameState) || !IsValid(TransferComponent))
	{
		UpdateStatusText(TEXT("사진 데이터를 불러올 수 없습니다."));
		return;
	}

	PendingPhotoIds = GameState->GetSelectedPhotoIds(ViewedPlayerState);
	if (PendingPhotoIds.IsEmpty())
	{
		UpdateStatusText(TEXT("고른 사진이 없습니다."));
		return;
	}

	TransferComponent->OnPhotoTextureReceived.AddUniqueDynamic(
		this,
		&ThisClass::HandlePhotoTextureReceived);

	RequestNextPhoto();
}

void UNPResultPicturePopup::HandleCloseClicked()
{
	RemoveFromParent();
}

void UNPResultPicturePopup::HandleThumbnail0Clicked() { HandleThumbnailClicked(0); }
void UNPResultPicturePopup::HandleThumbnail1Clicked() { HandleThumbnailClicked(1); }
void UNPResultPicturePopup::HandleThumbnail2Clicked() { HandleThumbnailClicked(2); }
void UNPResultPicturePopup::HandleThumbnail3Clicked() { HandleThumbnailClicked(3); }
void UNPResultPicturePopup::HandleThumbnail4Clicked() { HandleThumbnailClicked(4); }

void UNPResultPicturePopup::HandleThumbnailClicked(const int32 PictureIndex)
{
	ShowPicture(PictureIndex);
}

void UNPResultPicturePopup::RequestNextPhoto()
{
	if (!IsValid(TransferComponent) || DownloadingPhotoId.IsValid())
	{
		return;
	}

	while (!PendingPhotoIds.IsEmpty())
	{
		const FGuid NextPhotoId = PendingPhotoIds[0];
		PendingPhotoIds.RemoveAt(0);
		if (!NextPhotoId.IsValid())
		{
			continue;
		}

		DownloadingPhotoId = NextPhotoId;
		if (UTexture2D* CachedTexture = TransferComponent->FindReceivedPhoto(NextPhotoId))
		{
			HandlePhotoTextureReceived(NextPhotoId, CachedTexture);
			return;
		}

		TransferComponent->RequestPhoto(NextPhotoId);
		return;
	}
}

void UNPResultPicturePopup::HandlePhotoTextureReceived(
	const FGuid PhotoId,
	UTexture2D* Texture)
{
	if (PhotoId != DownloadingPhotoId || !IsValid(Texture))
	{
		return;
	}

	DownloadingPhotoId.Invalidate();
	const int32 PictureIndex = PictureTextures.Add(Texture);

	if (ThumbnailImages.IsValidIndex(PictureIndex)
		&& IsValid(ThumbnailImages[PictureIndex]))
	{
		ThumbnailImages[PictureIndex]->SetBrushFromTexture(Texture);
	}

	if (ThumbnailButtons.IsValidIndex(PictureIndex)
		&& IsValid(ThumbnailButtons[PictureIndex]))
	{
		ThumbnailButtons[PictureIndex]->SetIsEnabled(true);
	}

	RequestNextPhoto();
}

void UNPResultPicturePopup::ShowPicture(const int32 PictureIndex)
{
	if (!PictureTextures.IsValidIndex(PictureIndex)
		|| !IsValid(PreviewPopupWidgetClass))
	{
		return;
	}

	UNPResultPicturePreviewPopup* PreviewPopup =
		CreateWidget<UNPResultPicturePreviewPopup>(
			GetOwningPlayer(),
			PreviewPopupWidgetClass);
	if (!IsValid(PreviewPopup))
	{
		return;
	}

	PreviewPopup->AddToViewport(200);
	PreviewPopup->OpenWithTexture(PictureTextures[PictureIndex]);
}

void UNPResultPicturePopup::UpdateStatusText(const FString& Text) const
{
	if (IsValid(PlayerNameText))
	{
		PlayerNameText->SetText(FText::FromString(Text));
	}
}
