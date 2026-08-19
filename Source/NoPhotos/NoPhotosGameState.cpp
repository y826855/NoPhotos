#include "NoPhotosGameState.h"

#include "Net/UnrealNetwork.h"

void ANoPhotosGameState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANoPhotosGameState, PhotoEvidence);
	DOREPLIFETIME(ANoPhotosGameState, TransferredPhotoIds);
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
