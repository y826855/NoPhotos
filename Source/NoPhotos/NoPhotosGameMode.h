// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Gameplay/Photo/NPPhotoEvidenceTypes.h"
#include "NoPhotosGameMode.generated.h"

class UNPMatchScorePolicy;
class UNPPhotoEvidenceService;

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class ANoPhotosGameMode : public AGameModeBase
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
};



