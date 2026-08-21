#pragma once

#include "CoreMinimal.h"
#include "Core/Main/NPMainGameState.h"
#include "Gameplay/Photo/NPPhotoEvidenceTypes.h"
#include "NoPhotosGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNoPhotosPhotoEvidenceChanged);

/** 실제 게임 매치의 사진 증거 상태를 모든 클라이언트에 복제합니다. */
UCLASS()
class NOPHOTOS_API ANoPhotosGameState : public ANPMainGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Photo")
	TArray<FNPReplicatedPhotoEvidence> GetPhotoEvidence() const { return PhotoEvidence; }

	UFUNCTION(BlueprintPure, Category="Photo")
	TArray<FGuid> GetTransferredPhotoIds() const { return TransferredPhotoIds; }

	UFUNCTION(BlueprintPure, Category="Photo")
	TArray<FGuid> GetSelectedPhotoIds(const APlayerState* PlayerState) const;

	UPROPERTY(BlueprintAssignable, Category="Photo")
	FOnNoPhotosPhotoEvidenceChanged OnPhotoEvidenceChanged;

	void AddPhotoEvidence(const FNPPhotoEvidenceResult& Result, int32 AwardedScore);
	void RegisterTransferredPhoto(const FGuid& PhotoId);
	void AttachPhotoId(APlayerState* Photographer, uint16 CaptureSequence, const FGuid& PhotoId);
	void SetSelectedPhotoIds(APlayerState* PlayerState, const TArray<FGuid>& PhotoIds);

private:
	UFUNCTION()
	void OnRep_PhotoEvidence();

	UFUNCTION()
	void OnRep_TransferredPhotoIds();

	UPROPERTY(ReplicatedUsing=OnRep_PhotoEvidence)
	TArray<FNPReplicatedPhotoEvidence> PhotoEvidence;

	UPROPERTY(ReplicatedUsing=OnRep_TransferredPhotoIds)
	TArray<FGuid> TransferredPhotoIds;

	UPROPERTY(ReplicatedUsing=OnRep_SelectedPhotos)
	TArray<FNPPlayerSelectedPhotos> SelectedPhotos;

	UFUNCTION()
	void OnRep_SelectedPhotos();
};
