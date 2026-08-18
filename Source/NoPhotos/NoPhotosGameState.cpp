#include "NoPhotosGameState.h"

#include "Net/UnrealNetwork.h"

void ANoPhotosGameState::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANoPhotosGameState, PhotoEvidence);
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
	NewEvidence.Photographer = Result.Photographer;
	NewEvidence.Thief = Result.Thief;
	NewEvidence.Relic = Result.Relic;
	NewEvidence.AwardedScore = AwardedScore;
	NewEvidence.ServerCaptureTime = Result.ServerCaptureTime;
	ForceNetUpdate();
	OnPhotoEvidenceChanged.Broadcast();
}

void ANoPhotosGameState::OnRep_PhotoEvidence()
{
	OnPhotoEvidenceChanged.Broadcast();
}
