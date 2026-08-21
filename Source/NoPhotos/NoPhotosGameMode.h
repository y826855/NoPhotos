// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Main/NPMainGameMode.h"
#include "Gameplay/Photo/NPPhotoEvidenceTypes.h"
#include "NoPhotosGameMode.generated.h"

class UNPMatchScorePolicy;
class UNPPhotoEvidenceService;
class UNPPhotoRepository;
class UNPRelicDeliveryService;
class APlayerController;

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ANoPhotosGameMode : public ANPMainGameMode
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	ANoPhotosGameMode();
	virtual void InitGame(
		const FString& MapName,
		const FString& Options,
		FString& ErrorMessage) override;

	FNPPhotoEvidenceResult HandlePhotoCaptureRequest(const FNPPhotoCaptureRequest& Request);
	UNPPhotoRepository* GetPhotoRepository() const { return PhotoRepository; }
	UNPRelicDeliveryService* GetRelicDeliveryService() const { return RelicDeliveryService; }
	void HandlePhotoStored(APlayerController* Photographer, uint16 CaptureSequence, const FGuid& PhotoId);

protected:
	UPROPERTY(EditDefaultsOnly, Category="Photo")
	TSubclassOf<UNPPhotoEvidenceService> PhotoEvidenceServiceClass;

	UPROPERTY(EditDefaultsOnly, Category="Photo")
	TSubclassOf<UNPMatchScorePolicy> MatchScorePolicyClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNPPhotoEvidenceService> PhotoEvidenceService;

	UPROPERTY(Transient)
	TObjectPtr<UNPMatchScorePolicy> MatchScorePolicy;

	UPROPERTY(Transient)
	TObjectPtr<UNPPhotoRepository> PhotoRepository;

	UPROPERTY(Transient)
	TObjectPtr<UNPRelicDeliveryService> RelicDeliveryService;
};



