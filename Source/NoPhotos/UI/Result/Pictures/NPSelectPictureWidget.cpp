#include "UI/Result/Pictures/NPSelectPictureWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/Main/NPMainPlayerController.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/Photo/NPPhotoTransferComponent.h"
#include "NoPhotosGameState.h"
#include "NoPhotosPlayerController.h"
#include "UI/Result/Pictures/NPPictureList.h"
#include "UI/Result/Pictures/NPSelectedPictureListWidget.h"
#include "UI/Result/Pictures/NPShowPicture.h"

void UNPSelectPictureWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(PictureListWidget))
	{
		PictureListWidget->OnPictureClicked.AddUniqueDynamic(
			this,
			&UNPSelectPictureWidget::HandlePictureClicked);
	}

	if (IsValid(SelectedPictureListWidget))
	{
		SelectedPictureListWidget->OnPictureClicked.AddUniqueDynamic(
			this,
			&UNPSelectPictureWidget::HandleSelectedPictureClicked);
		SelectedPictureListWidget->OnPictureRemoveRequested.AddUniqueDynamic(
			this,
			&UNPSelectPictureWidget::HandleSelectedPictureRemoveRequested);
	}

	if (IsValid(ShowPictureWidget))
	{
		ShowPictureWidget->OnSelectRequested.AddUniqueDynamic(
			this,
			&UNPSelectPictureWidget::HandleSelectRequested);
	}

	if (IsValid(NextButton))
	{
		NextButton->OnClicked.AddUniqueDynamic(
			this,
			&UNPSelectPictureWidget::HandleNextButtonClicked);
	}

	if (ANoPhotosPlayerController* Controller =
		Cast<ANoPhotosPlayerController>(GetOwningPlayer()))
	{
		TransferComponent = Controller->GetPhotoTransferComponent();
		if (IsValid(TransferComponent))
		{
			TransferComponent->OnPhotoTextureReceived.AddUniqueDynamic(
				this,
				&UNPSelectPictureWidget::HandlePhotoTextureReceived);
		}
	}

	ObservedGameState = GetWorld()
		? GetWorld()->GetGameState<ANoPhotosGameState>()
		: nullptr;

	if (IsValid(ObservedGameState))
	{
		ObservedGameState->OnPhotoEvidenceChanged.AddUniqueDynamic(
			this,
			&UNPSelectPictureWidget::HandlePhotoEvidenceChanged);
	}

	UpdateSelectedPictureCountText();
	RequestOwnedPictures();
}

void UNPSelectPictureWidget::NativeDestruct()
{
	if (IsValid(TransferComponent))
	{
		TransferComponent->OnPhotoTextureReceived.RemoveDynamic(
			this,
			&UNPSelectPictureWidget::HandlePhotoTextureReceived);
	}

	if (IsValid(ObservedGameState))
	{
		ObservedGameState->OnPhotoEvidenceChanged.RemoveDynamic(
			this,
			&UNPSelectPictureWidget::HandlePhotoEvidenceChanged);
	}

	Super::NativeDestruct();
}

void UNPSelectPictureWidget::InitializePictures(
	const TArray<UTexture2D*>& InPictures)
{
	if (!IsValid(PictureListWidget))
	{
		return;
	}

	PictureListWidget->ClearPictures();

	if (IsValid(SelectedPictureListWidget))
	{
		SelectedPictureListWidget->ClearSelectedPictures();
	}

	PictureTextures.Empty();
	PicturePhotoIds.Empty();
	CurrentPictureIndex = INDEX_NONE;

	for (UTexture2D* PictureTexture : InPictures)
	{
		if (!IsValid(PictureTexture))
		{
			continue;
		}

		PictureTextures.Add(PictureTexture);
		PicturePhotoIds.Add(FGuid());
		PictureListWidget->AddPicture(PictureTexture);
	}

	if (!PictureTextures.IsEmpty())
	{
		ShowPicture(0);
	}

	UpdateSelectedPictureCountText();
}

TArray<FGuid> UNPSelectPictureWidget::GetSelectedPhotoIds() const
{
	TArray<FGuid> SelectedPhotoIds;

	for (const int32 PictureIndex : GetSelectedPictureIndices())
	{
		if (PicturePhotoIds.IsValidIndex(PictureIndex)
			&& PicturePhotoIds[PictureIndex].IsValid())
		{
			SelectedPhotoIds.Add(PicturePhotoIds[PictureIndex]);
		}
	}

	return SelectedPhotoIds;
}

void UNPSelectPictureWidget::HandlePictureClicked(
	const int32 PictureIndex)
{
	ShowPicture(PictureIndex);
}

void UNPSelectPictureWidget::HandleSelectRequested(
	const int32 PictureIndex)
{
	if (!IsValid(PictureListWidget)
		|| !PictureTextures.IsValidIndex(PictureIndex))
	{
		return;
	}

	const bool bWasSelected =
		PictureListWidget->IsPictureSelected(PictureIndex);

	if (bWasSelected)
	{
		PictureListWidget->SetPictureSelected(PictureIndex, false);
		if (IsValid(SelectedPictureListWidget))
		{
			SelectedPictureListWidget->RemoveSelectedPicture(PictureIndex);
		}
		ShowPictureWidget->SetSelected(false);
		UpdateSelectedPictureCountText();
		return;
	}

	if (PictureListWidget->GetSelectedPictureCount()
		>= MaxSelectedPictureCount)
	{
		return;
	}

	PictureListWidget->SetPictureSelected(PictureIndex, true);
	if (IsValid(SelectedPictureListWidget))
	{
		SelectedPictureListWidget->AddSelectedPicture(
			PictureTextures[PictureIndex],
			PictureIndex);
	}
	ShowPictureWidget->SetSelected(true);
	UpdateSelectedPictureCountText();
}

void UNPSelectPictureWidget::HandleSelectedPictureClicked(
	const int32 PictureIndex)
{
	ShowPicture(PictureIndex);
}

void UNPSelectPictureWidget::HandleSelectedPictureRemoveRequested(
	const int32 PictureIndex)
{
	if (!IsValid(PictureListWidget)
		|| !PictureTextures.IsValidIndex(PictureIndex)
		|| !PictureListWidget->IsPictureSelected(PictureIndex))
	{
		return;
	}

	PictureListWidget->SetPictureSelected(PictureIndex, false);

	if (IsValid(SelectedPictureListWidget))
	{
		SelectedPictureListWidget->RemoveSelectedPicture(PictureIndex);
	}

	if (CurrentPictureIndex == PictureIndex && IsValid(ShowPictureWidget))
	{
		ShowPictureWidget->SetSelected(false);
	}

	UpdateSelectedPictureCountText();
}

void UNPSelectPictureWidget::HandleNextButtonClicked()
{
	if (bWaitingForOtherPlayers)
	{
		return;
	}

	ANPMainPlayerController* MainPlayerController =
		Cast<ANPMainPlayerController>(GetOwningPlayer());

	if (!IsValid(MainPlayerController))
	{
		return;
	}

	bWaitingForOtherPlayers = true;
	
	if (IsValid(NextButton))
	{
		NextButton->SetIsEnabled(false);
	}

	if (IsValid(SelectedPictureCountText))
	{
		SelectedPictureCountText->SetText(FText::FromString(TEXT("대기 중...")));
	}

	BP_OnSelectionConfirmed(GetSelectedPictureIndices());
	MainPlayerController->ServerConfirmPictureSelection(GetSelectedPhotoIds());
}

void UNPSelectPictureWidget::HandlePhotoEvidenceChanged()
{
	RequestOwnedPictures();
}

void UNPSelectPictureWidget::RequestOwnedPictures()
{
	if (!IsValid(ObservedGameState))
	{
		ObservedGameState = GetWorld()
			? GetWorld()->GetGameState<ANoPhotosGameState>()
			: nullptr;

		if (IsValid(ObservedGameState))
		{
			ObservedGameState->OnPhotoEvidenceChanged.AddUniqueDynamic(
				this,
				&UNPSelectPictureWidget::HandlePhotoEvidenceChanged);
		}
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	APlayerState* LocalPlayerState =
		IsValid(OwningPlayer) ? OwningPlayer->PlayerState : nullptr;

	if (!IsValid(ObservedGameState) || !IsValid(LocalPlayerState))
	{
		return;
	}

	for (const FNPReplicatedPhotoEvidence& Evidence :
		ObservedGameState->GetPhotoEvidence())
	{
		if (Evidence.Photographer != LocalPlayerState
			|| !Evidence.PhotoId.IsValid()
			|| RequestedPhotoIds.Contains(Evidence.PhotoId))
		{
			continue;
		}

		RequestedPhotoIds.Add(Evidence.PhotoId);
		PendingPhotoIds.Add(Evidence.PhotoId);
	}

	RequestNextPicture();
}

void UNPSelectPictureWidget::RequestNextPicture()
{
	if (!IsValid(TransferComponent)
		|| !IsValid(PictureListWidget)
		|| DownloadingPhotoId.IsValid())
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

		if (UTexture2D* CachedTexture =
			TransferComponent->FindReceivedPhoto(DownloadingPhotoId))
		{
			HandlePhotoTextureReceived(DownloadingPhotoId, CachedTexture);
			return;
		}

		TransferComponent->RequestPhoto(DownloadingPhotoId);
		return;
	}
}

void UNPSelectPictureWidget::HandlePhotoTextureReceived(
	const FGuid PhotoId,
	UTexture2D* Texture)
{
	if (PhotoId != DownloadingPhotoId || !IsValid(Texture))
	{
		return;
	}

	DownloadingPhotoId.Invalidate();

	PictureTextures.Add(Texture);
	PicturePhotoIds.Add(PhotoId);

	if (IsValid(PictureListWidget))
	{
		PictureListWidget->AddPicture(Texture);
	}

	if (CurrentPictureIndex == INDEX_NONE)
	{
		ShowPicture(0);
	}

	UpdateSelectedPictureCountText();
	RequestNextPicture();
}

void UNPSelectPictureWidget::ShowPicture(
	const int32 PictureIndex)
{
	if (!PictureTextures.IsValidIndex(PictureIndex)	|| !IsValid(ShowPictureWidget))
	{
		return;
	}

	CurrentPictureIndex = PictureIndex;

	ShowPictureWidget->SetPicture(PictureTextures[PictureIndex],	PictureIndex);

	if (IsValid(PictureListWidget))
	{
		ShowPictureWidget->SetSelected(PictureListWidget->IsPictureSelected(PictureIndex));
	}
}

void UNPSelectPictureWidget::UpdateSelectedPictureCountText()
{
	if (!IsValid(SelectedPictureCountText)
		|| !IsValid(PictureListWidget))
	{
		return;
	}

	const int32 SelectedCount =	PictureListWidget->GetSelectedPictureCount();

	SelectedPictureCountText->SetText(
		FText::FromString(FString::Printf(TEXT("%d / %d"), SelectedCount, MaxSelectedPictureCount)));
}

TArray<int32> UNPSelectPictureWidget::GetSelectedPictureIndices() const
{
	TArray<int32> SelectedIndices;

	if (!IsValid(PictureListWidget))
	{
		return SelectedIndices;
	}

	for (int32 PictureIndex = 0; PictureIndex < PictureTextures.Num(); ++PictureIndex)
	{
		if (PictureListWidget->IsPictureSelected(PictureIndex))
		{
			SelectedIndices.Add(PictureIndex);
		}
	}

	return SelectedIndices;
}
