// Copyright Epic Games, Inc. All Rights Reserved.

#include "NoPhotosGameMode.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/Photo/NPPhotoEvidenceService.h"
#include "Gameplay/Photo/NPPhotoLog.h"
#include "Gameplay/Photo/NPPhotoRepository.h"
#include "Gameplay/Relic/NPRelicDeliveryService.h"
#include "NoPhotosGameState.h"

ANoPhotosGameMode::ANoPhotosGameMode()
{
	GameStateClass = ANoPhotosGameState::StaticClass();
	PhotoEvidenceServiceClass = UNPPhotoEvidenceService::StaticClass();
}

void ANoPhotosGameMode::InitGame(
	const FString& MapName,
	const FString& Options,
	FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UClass* EvidenceClass = PhotoEvidenceServiceClass
		? PhotoEvidenceServiceClass.Get()
		: UNPPhotoEvidenceService::StaticClass();
	PhotoEvidenceService = NewObject<UNPPhotoEvidenceService>(
		this, EvidenceClass, TEXT("PhotoEvidenceService"));
	PhotoEvidenceService->Initialize(this);

	PhotoRepository = NewObject<UNPPhotoRepository>(this, TEXT("PhotoRepository"));
	PhotoRepository->Initialize(this);

	RelicDeliveryService = NewObject<UNPRelicDeliveryService>(this, TEXT("RelicDeliveryService"));
	RelicDeliveryService->Initialize(this);
}

FNPPhotoEvidenceResult ANoPhotosGameMode::HandlePhotoCaptureRequest(
	const FNPPhotoCaptureRequest& Request)
{
	FNPPhotoEvidenceResult Result;
	Result.CaptureSequence = Request.CaptureSequence;
	if (!HasAuthority() || !PhotoEvidenceService)
	{
		Result.FailureReason = ENPPhotoEvidenceFailureReason::InvalidPhotographer;
		return Result;
	}

	Result = PhotoEvidenceService->EvaluatePhoto(Request);
	if (PhotoRepository
		&& (Result.bSuccess
			|| Result.bReactiveTargetSuccess
			|| Result.FailureReason == ENPPhotoEvidenceFailureReason::NoValidEvidence))
	{
		PhotoRepository->AuthorizeCapture(Request.Photographer, Request.CaptureSequence);
	}
	if (!Result.bSuccess && !Result.bReactiveTargetSuccess)
	{
		UE_LOG(
			LogNPPhoto,
			Warning,
			TEXT("[GameMode] Photo evidence rejected. Reason=%d"),
			static_cast<int32>(Result.FailureReason));
		return Result;
	}

	if (Result.bSuccess && RelicDeliveryService)
	{
		RelicDeliveryService->RegisterPhotoEvidence(Result);
	}

	if (Result.bSuccess)
	{
		if (ANoPhotosGameState* PhotoGameState = GetGameState<ANoPhotosGameState>())
		{
			PhotoGameState->AddPhotoEvidence(Result, 0);
		}
	}
	UE_LOG(
		LogNPPhoto,
		Log,
		TEXT("[GameMode] Photo accepted. Photographer=%s RelicEvidence=%s Thief=%s Relic=%s ReactiveTarget=%s"),
		*GetNameSafe(Result.Photographer.Get()),
		Result.bSuccess ? TEXT("true") : TEXT("false"),
		*GetNameSafe(Result.Thief.Get()),
		*GetNameSafe(Result.Relic.Get()),
		*GetNameSafe(Result.ReactiveTarget.Get()));
	return Result;
}

void ANoPhotosGameMode::HandlePhotoStored(
	APlayerController* Photographer,
	const uint16 CaptureSequence,
	const FGuid& PhotoId)
{
	if (ANoPhotosGameState* PhotoGameState = GetGameState<ANoPhotosGameState>())
	{
		PhotoGameState->RegisterTransferredPhoto(PhotoId);
		PhotoGameState->AttachPhotoId(
			Photographer ? Photographer->PlayerState : nullptr,
			CaptureSequence,
			PhotoId);
	}
}
