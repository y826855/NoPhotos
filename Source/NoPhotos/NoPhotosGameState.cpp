#include "NoPhotosGameState.h"

#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

void ANoPhotosGameState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANoPhotosGameState, PhotoEvidence);
	DOREPLIFETIME(ANoPhotosGameState, TransferredPhotoIds);
	DOREPLIFETIME(ANoPhotosGameState, SelectedPhotos);
}

TArray<FGuid> ANoPhotosGameState::GetSelectedPhotoIds(
	const APlayerState* PlayerState) const
{
	if (!IsValid(PlayerState))
	{
		return {};
	}

	for (const FNPPlayerSelectedPhotos& Selected : SelectedPhotos)
	{
		if (Selected.PlayerState == PlayerState)
		{
			return Selected.PhotoIds;
		}
	}

	return {};
}

void ANoPhotosGameState::SetSelectedPhotoIds(
	APlayerState* PlayerState,
	const TArray<FGuid>& PhotoIds)
{
	if (!HasAuthority() || !IsValid(PlayerState))
	{
		return;
	}

	for (FNPPlayerSelectedPhotos& Selected : SelectedPhotos)
	{
		if (Selected.PlayerState == PlayerState)
		{
			Selected.PhotoIds = PhotoIds;
			ForceNetUpdate();
			OnPhotoEvidenceChanged.Broadcast();
			return;
		}
	}

	FNPPlayerSelectedPhotos& NewSelected = SelectedPhotos.AddDefaulted_GetRef();
	NewSelected.PlayerState = PlayerState;
	NewSelected.PhotoIds = PhotoIds;
	ForceNetUpdate();
	OnPhotoEvidenceChanged.Broadcast();
}

void ANoPhotosGameState::AddPhotoEvidence(
	const FNPPhotoEvidenceResult& Result,
	const int32 AwardedScore)
{
	if (!HasAuthority() || !Result.bSuccess)
	{
		return;
	}

	FNPReplicatedPhotoEvidence& NewEvidence = PhotoEvidence.AddDefaulted_GetRef();
	NewEvidence.CaptureSequence = Result.CaptureSequence;
	NewEvidence.Photographer = Result.Photographer;
	NewEvidence.Thief = Result.Thief;
	NewEvidence.Relic = Result.Relic;
	NewEvidence.AwardedScore = AwardedScore;
	NewEvidence.ServerCaptureTime = Result.ServerCaptureTime;
	if (PhotoEvidence.Num() > MaximumStoredPhotos)
	{
		PhotoEvidence.RemoveAt(0, PhotoEvidence.Num() - MaximumStoredPhotos);
	}
	ForceNetUpdate();
	OnPhotoEvidenceChanged.Broadcast();
}

void ANoPhotosGameState::RegisterTransferredPhoto(const FGuid& PhotoId)
{
	if (!HasAuthority() || !PhotoId.IsValid() || TransferredPhotoIds.Contains(PhotoId))
	{
		return;
	}
	TransferredPhotoIds.Add(PhotoId);
	if (TransferredPhotoIds.Num() > MaximumStoredPhotos)
	{
		TransferredPhotoIds.RemoveAt(
			0,
			TransferredPhotoIds.Num() - MaximumStoredPhotos);
	}
	ForceNetUpdate();
	OnPhotoEvidenceChanged.Broadcast();
}

void ANoPhotosGameState::AttachPhotoId(
	APlayerState* Photographer,
	const uint16 CaptureSequence,
	const FGuid& PhotoId)
{
	if (!HasAuthority() || !PhotoId.IsValid())
	{
		return;
	}

	for (FNPReplicatedPhotoEvidence& Evidence : PhotoEvidence)
	{
		if (Evidence.Photographer == Photographer
			&& Evidence.CaptureSequence == CaptureSequence)
		{
			Evidence.PhotoId = PhotoId;
			ForceNetUpdate();
			OnPhotoEvidenceChanged.Broadcast();
			return;
		}
	}
}

void ANoPhotosGameState::OnRep_PhotoEvidence()
{
	OnPhotoEvidenceChanged.Broadcast();
}

void ANoPhotosGameState::OnRep_TransferredPhotoIds()
{
	OnPhotoEvidenceChanged.Broadcast();
}

void ANoPhotosGameState::OnRep_SelectedPhotos()
{
	OnPhotoEvidenceChanged.Broadcast();
}
