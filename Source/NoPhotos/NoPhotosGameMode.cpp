// Copyright Epic Games, Inc. All Rights Reserved.

#include "NoPhotosGameMode.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Core/NPPlayerState.h"
#include "Gameplay/Photo/NPMatchScorePolicy.h"
#include "Gameplay/Photo/NPPhotoEvidenceService.h"
#include "Gameplay/Photo/NPPhotoLog.h"
#include "Gameplay/Photo/NPPhotoRepository.h"
#include "Gameplay/Relic/NPRelicDeliveryService.h"
#include "NoPhotosGameState.h"

ANoPhotosGameMode::ANoPhotosGameMode()
{
	GameStateClass = ANoPhotosGameState::StaticClass();
	PhotoEvidenceServiceClass = UNPPhotoEvidenceService::StaticClass();
	MatchScorePolicyClass = UNPMatchScorePolicy::StaticClass();
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

	UClass* ScoreClass = MatchScorePolicyClass
		? MatchScorePolicyClass.Get()
		: UNPMatchScorePolicy::StaticClass();
	MatchScorePolicy = NewObject<UNPMatchScorePolicy>(
		this, ScoreClass, TEXT("MatchScorePolicy"));

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
	if (!HasAuthority() || !PhotoEvidenceService || !MatchScorePolicy)
	{
		Result.FailureReason = ENPPhotoEvidenceFailureReason::InvalidPhotographer;
		return Result;
	}

	Result = PhotoEvidenceService->EvaluatePhoto(Request);
	if (PhotoRepository
		&& (Result.bSuccess
			|| Result.FailureReason == ENPPhotoEvidenceFailureReason::NoValidEvidence))
	{
		PhotoRepository->AuthorizeCapture(Request.Photographer, Request.CaptureSequence);
	}
	if (!Result.bSuccess)
	{
		UE_LOG(
			LogNPPhoto,
			Warning,
			TEXT("[GameMode] Photo evidence rejected. Reason=%d"),
			static_cast<int32>(Result.FailureReason));
		return Result;
	}

	if (RelicDeliveryService)
	{
		RelicDeliveryService->RegisterPhotoEvidence(Result);
	}

	const int32 AwardedScore = MatchScorePolicy->CalculateEvidenceScore(Result);
	if (ANPPlayerState* PhotographerPlayerState = Cast<ANPPlayerState>(Result.Photographer))
	{
		PhotographerPlayerState->AddScore(AwardedScore);
	}
	else if (IsValid(Result.Photographer))
	{
		// 커스텀 PlayerState가 아닌 테스트 환경에서도 기존 동작을 유지합니다.
		Result.Photographer->SetScore(Result.Photographer->GetScore() + AwardedScore);
	}
	if (ANoPhotosGameState* PhotoGameState = GetGameState<ANoPhotosGameState>())
	{
		PhotoGameState->AddPhotoEvidence(Result, AwardedScore);
	}
	UE_LOG(
		LogNPPhoto,
		Log,
		TEXT("[GameMode] Photo evidence accepted. Photographer=%s Thief=%s Relic=%s Score=%d"),
		*GetNameSafe(Result.Photographer),
		*GetNameSafe(Result.Thief),
		*GetNameSafe(Result.Relic),
		AwardedScore);
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
