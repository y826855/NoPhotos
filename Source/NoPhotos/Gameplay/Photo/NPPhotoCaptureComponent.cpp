#include "Gameplay/Photo/NPPhotoCaptureComponent.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/Character/NPStablePhysicsPawn.h"
#include "Gameplay/Character/Component/NPStablePhysicsGrabComponent.h"
#include "Gameplay/Photo/NPPhotoLog.h"
#include "Gameplay/Photo/NPPhotoImageCodec.h"
#include "Gameplay/Photo/NPPhotoTransferComponent.h"
#include "NoPhotosGameMode.h"
#include "NoPhotosPlayerController.h"
#include "TimerManager.h"

UNPPhotoCaptureComponent::UNPPhotoCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNPPhotoCaptureComponent::BeginPlay()
{
	Super::BeginPlay();
	ImageCodec = NewObject<UNPPhotoImageCodec>(this, TEXT("PhotoCaptureImageCodec"));

	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (PlayerController && PlayerController->IsLocalController())
	{
		InitializeLocalCapture();
	}
}

void UNPPhotoCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ExitPhotoMode();
	ReleaseMovementLock();
	Super::EndPlay(EndPlayReason);
}

void UNPPhotoCaptureComponent::TogglePhotoMode()
{
	if (bPhotoModeActive)
	{
		ExitPhotoMode();
	}
	else
	{
		EnterPhotoMode();
	}
}

bool UNPPhotoCaptureComponent::EnterPhotoMode()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	ANPStablePhysicsPawn* StablePawn = PlayerController
		? Cast<ANPStablePhysicsPawn>(PlayerController->GetPawn())
		: nullptr;
	if (!PlayerController || !PlayerController->IsLocalController() || !StablePawn)
	{
		UE_LOG(LogNPPhoto, Warning, TEXT("[PhotoMode] Enter rejected: invalid local Pawn."));
		return false;
	}
	if (IsPhotographerGrabbing())
	{
		UE_LOG(LogNPPhoto, Warning, TEXT("[PhotoMode] Enter rejected: photographer is grabbing."));
		return false;
	}

	PhotoModePawn = StablePawn;
	bPhotoModeActive = true;
	StablePawn->SetPhotoViewActive(true);
	ServerSetPhotoModeActive(true);
	UE_LOG(LogNPPhoto, Log, TEXT("[PhotoMode] Entered. Pawn=%s"), *GetNameSafe(StablePawn));
	return true;
}

void UNPPhotoCaptureComponent::ExitPhotoMode()
{
	if (PhotoModePawn.IsValid())
	{
		PhotoModePawn->SetPhotoViewActive(false);
	}
	PhotoModePawn.Reset();
	if (bPhotoModeActive)
	{
		bPhotoModeActive = false;
		ServerSetPhotoModeActive(false);
		UE_LOG(LogNPPhoto, Log, TEXT("[PhotoMode] Exited."));
	}
}

bool UNPPhotoCaptureComponent::TakePhoto()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	UWorld* World = GetWorld();
	if (!PlayerController || !PlayerController->IsLocalController() || !World)
	{
		UE_LOG(
			LogNPPhoto,
			Error,
			TEXT("[Capture] Invalid local capture context. Controller=%s Local=%s World=%s"),
			*GetNameSafe(PlayerController),
			PlayerController && PlayerController->IsLocalController() ? TEXT("true") : TEXT("false"),
			*GetNameSafe(World));
		return false;
	}
	ANPStablePhysicsPawn* Pawn = Cast<ANPStablePhysicsPawn>(PlayerController->GetPawn());
	if (!bPhotoModeActive || !Pawn || !Pawn->IsPhotoViewReady())
	{
		UE_LOG(
			LogNPPhoto,
			Warning,
			TEXT("[Capture] Rejected locally: photo mode is inactive or camera is blending. Active=%s Pawn=%s Ready=%s"),
			bPhotoModeActive ? TEXT("true") : TEXT("false"),
			*GetNameSafe(Pawn),
			Pawn && Pawn->IsPhotoViewReady() ? TEXT("true") : TEXT("false"));
		return false;
	}

	const double CurrentTime = World->GetTimeSeconds();
	if (IsPhotographerGrabbing())
	{
		UE_LOG(LogNPPhoto, Warning, TEXT("[Capture] Rejected locally: photographer is grabbing an object."));
		return false;
	}
	const double ElapsedSinceLastCapture = CurrentTime - LastLocalCaptureTime;
	if (ElapsedSinceLastCapture < PhotoCooldown)
	{
		UE_LOG(
			LogNPPhoto,
			Warning,
			TEXT("[Capture] Rejected locally: cooldown. Remaining=%.2f"),
			PhotoCooldown - ElapsedSinceLastCapture);
		return false;
	}

	if (!SceneCapture || !PhotoRenderTarget)
	{
		UE_LOG(LogNPPhoto, Log, TEXT("[Capture] Initializing SceneCapture and RenderTarget."));
		InitializeLocalCapture();
	}
	if (!SceneCapture || !PhotoRenderTarget)
	{
		UE_LOG(
			LogNPPhoto,
			Error,
			TEXT("[Capture] Initialization failed. SceneCapture=%s RenderTarget=%s"),
			*GetNameSafe(SceneCapture),
			*GetNameSafe(PhotoRenderTarget));
		return false;
	}

	bPhotoAttemptInProgress = true;
	if (!Pawn || !Pawn->PlayPhotoShotMontage())
	{
		UE_LOG(
			LogNPPhoto,
			Warning,
			TEXT("[Montage] Photo attempt canceled. Pawn=%s"),
			*GetNameSafe(PlayerController->GetPawn()));
		CancelPhotoAttempt();
		return false;
	}
	UE_LOG(LogNPPhoto, Log, TEXT("[Montage] PhotoShotMontage started. Pawn=%s"), *GetNameSafe(Pawn));

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
	SceneCapture->SetWorldLocationAndRotation(CameraLocation, CameraRotation);
	UE_LOG(
		LogNPPhoto,
		Log,
		TEXT("[Capture] Capturing scene. Location=%s Rotation=%s Target=%s"),
		*CameraLocation.ToCompactString(),
		*CameraRotation.ToCompactString(),
		*GetNameSafe(PhotoRenderTarget));
	SceneCapture->CaptureScene();
	UE_LOG(LogNPPhoto, Log, TEXT("[Capture] CaptureScene requested successfully."));
	OnPhotoCaptured.Broadcast(PhotoRenderTarget);
	if (ANoPhotosPlayerController* NoPhotosPlayerController = Cast<ANoPhotosPlayerController>(PlayerController))
	{
		NoPhotosPlayerController->PlayPhotoFlash();
	}
	else
	{
		UE_LOG(
			LogNPPhoto,
			Warning,
			TEXT("[PhotoUI] Flash skipped: Controller is not ANoPhotosPlayerController. Controller=%s"),
			*GetNameSafe(PlayerController));
	}

	LastLocalCaptureTime = CurrentTime;
	bPhotoAttemptInProgress = false;
	ApplyMovementLock();
	const uint16 CaptureSequence = ++NextCaptureSequence;
	if (ImageCodec)
	{
		TArray<uint8> JpegData;
		if (ImageCodec->EncodeRenderTargetToJpeg(PhotoRenderTarget, JpegQuality, JpegData))
		{
			PendingJpegPhotos.Add(CaptureSequence, MoveTemp(JpegData));
		}
	}
	ServerRequestTakePhoto(CameraLocation, CameraRotation.Vector(), CaptureSequence);
	return true;
}

void UNPPhotoCaptureComponent::CancelPhotoAttempt()
{
	bPhotoAttemptInProgress = false;
}

bool UNPPhotoCaptureComponent::IsPhotographerGrabbing() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const UNPStablePhysicsGrabComponent* GrabComponent = Pawn
		? Pawn->FindComponentByClass<UNPStablePhysicsGrabComponent>()
		: nullptr;
	return GrabComponent && GrabComponent->IsHoldingObject();
}

void UNPPhotoCaptureComponent::ApplyMovementLock()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	UWorld* World = GetWorld();
	if (!PlayerController || !World || MovementLockDuration <= 0.0f)
	{
		return;
	}

	if (ANPStablePhysicsPawn* StablePawn = Cast<ANPStablePhysicsPawn>(PlayerController->GetPawn()))
	{
		if (MovementLockedPawn.IsValid() && MovementLockedPawn.Get() != StablePawn)
		{
			MovementLockedPawn->SetPhotoMovementLocked(false);
		}
		MovementLockedPawn = StablePawn;
		StablePawn->SetPhotoMovementLocked(true);
		bMovementLockApplied = true;
		UE_LOG(
			LogNPPhoto,
			Warning,
			TEXT("[MovementLock] APPLY Pawn=%s Locked=%s WorldTime=%.3f Duration=%.2f"),
			*GetNameSafe(StablePawn),
			StablePawn->IsPhotoMovementLocked() ? TEXT("true") : TEXT("false"),
			World->GetTimeSeconds(),
			MovementLockDuration);
	}
	else
	{
		UE_LOG(
			LogNPPhoto,
			Error,
			TEXT("[MovementLock] APPLY failed: controlled Pawn is not ANPStablePhysicsPawn. Pawn=%s"),
			*GetNameSafe(PlayerController->GetPawn()));
	}
	World->GetTimerManager().SetTimer(
		MovementUnlockTimer,
		this,
		&UNPPhotoCaptureComponent::ReleaseMovementLock,
		MovementLockDuration,
		false);
}

void UNPPhotoCaptureComponent::ReleaseMovementLock()
{
	if (!bMovementLockApplied)
	{
		return;
	}

	if (MovementLockedPawn.IsValid())
	{
		MovementLockedPawn->SetPhotoMovementLocked(false);
	}
	UE_LOG(
		LogNPPhoto,
		Warning,
		TEXT("[MovementLock] RELEASE Pawn=%s Valid=%s LockedAfterRelease=%s WorldTime=%.3f"),
		*GetNameSafe(MovementLockedPawn.Get()),
		MovementLockedPawn.IsValid() ? TEXT("true") : TEXT("false"),
		MovementLockedPawn.IsValid() && MovementLockedPawn->IsPhotoMovementLocked()
			? TEXT("true")
			: TEXT("false"),
		GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0);
	MovementLockedPawn.Reset();
	bMovementLockApplied = false;
}

void UNPPhotoCaptureComponent::InitializeLocalCapture()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || SceneCapture || PhotoRenderTarget)
	{
		UE_LOG(
			LogNPPhoto,
			Verbose,
			TEXT("[Capture] InitializeLocalCapture skipped. Owner=%s SceneCapture=%s RenderTarget=%s"),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(SceneCapture),
			*GetNameSafe(PhotoRenderTarget));
		return;
	}

	PhotoRenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("PhotoRenderTarget"));
	PhotoRenderTarget->RenderTargetFormat = RTF_RGBA8;
	PhotoRenderTarget->ClearColor = FLinearColor::Black;
	PhotoRenderTarget->InitAutoFormat(CaptureWidth, CaptureHeight);
	PhotoRenderTarget->UpdateResourceImmediate(true);

	SceneCapture = NewObject<USceneCaptureComponent2D>(OwnerActor, TEXT("PhotoSceneCapture"));
	OwnerActor->AddInstanceComponent(SceneCapture);
	SceneCapture->TextureTarget = PhotoRenderTarget;
	SceneCapture->FOVAngle = CaptureFOV;
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->RegisterComponent();
	UE_LOG(
		LogNPPhoto,
		Log,
		TEXT("[Capture] Local capture initialized. Size=%dx%d FOV=%.1f"),
		CaptureWidth,
		CaptureHeight,
		CaptureFOV);
}

void UNPPhotoCaptureComponent::ServerRequestTakePhoto_Implementation(
	FVector_NetQuantize10 CameraLocation,
	FVector_NetQuantizeNormal CameraForward,
	uint16 CaptureSequence)
{
	UE_LOG(
		LogNPPhoto,
		Log,
		TEXT("[Server] Photo RPC received. Owner=%s Sequence=%u"),
		*GetNameSafe(GetOwner()),
		CaptureSequence);

	APlayerController* Photographer = Cast<APlayerController>(GetOwner());
	ANoPhotosGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ANoPhotosGameMode>()
		: nullptr;
	if (!Photographer || !GameMode || !bServerPhotoModeActive)
	{
		UE_LOG(
			LogNPPhoto,
			Error,
			TEXT("[Server] Rejected: invalid photographer, GameMode, or photo mode. Photographer=%s GameMode=%s PhotoMode=%s"),
			*GetNameSafe(Photographer),
			*GetNameSafe(GameMode),
			bServerPhotoModeActive ? TEXT("true") : TEXT("false"));
		FNPPhotoEvidenceResult FailureResult;
		FailureResult.CaptureSequence = CaptureSequence;
		FailureResult.FailureReason = ENPPhotoEvidenceFailureReason::InvalidPhotographer;
		ClientReceivePhotoResult(FailureResult);
		return;
	}

	FNPPhotoEvidenceResult RejectedResult;
	RejectedResult.CaptureSequence = CaptureSequence;
	RejectedResult.Photographer = Photographer->PlayerState;
	if (IsPhotographerGrabbing())
	{
		UE_LOG(LogNPPhoto, Warning, TEXT("[Server] Rejected: photographer is grabbing an object."));
		RejectedResult.FailureReason = ENPPhotoEvidenceFailureReason::PhotographerIsGrabbing;
		ClientReceivePhotoResult(RejectedResult);
		return;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastServerCaptureTime < PhotoCooldown)
	{
		UE_LOG(
			LogNPPhoto,
			Warning,
			TEXT("[Server] Rejected: cooldown. Remaining=%.2f"),
			PhotoCooldown - (CurrentTime - LastServerCaptureTime));
		RejectedResult.FailureReason = ENPPhotoEvidenceFailureReason::CaptureOnCooldown;
		ClientReceivePhotoResult(RejectedResult);
		return;
	}
	LastServerCaptureTime = CurrentTime;
	ApplyMovementLock();

	if (ANPStablePhysicsPawn* PhotographerPawn = Cast<ANPStablePhysicsPawn>(Photographer->GetPawn()))
	{
		// Pawn은 모든 관련 클라이언트에 복제되므로 위치 기반 셔터음 멀티캐스트의 주체로 사용합니다.
		PhotographerPawn->BroadcastPhotoShutterSound(PhotographerPawn->GetActorLocation());
	}
	else
	{
		UE_LOG(
			LogNPPhoto,
			Warning,
			TEXT("[Audio] Shutter multicast skipped: photographer Pawn is invalid. Pawn=%s"),
			*GetNameSafe(Photographer->GetPawn()));
	}

	FNPPhotoCaptureRequest Request;
	Request.Photographer = Photographer;
	Request.CameraLocation = CameraLocation;
	Request.CameraForward = CameraForward;
	Request.CaptureSequence = CaptureSequence;
	ClientReceivePhotoResult(GameMode->HandlePhotoCaptureRequest(Request));
}

void UNPPhotoCaptureComponent::ServerSetPhotoModeActive_Implementation(const bool bActive)
{
	bServerPhotoModeActive = bActive;
}

void UNPPhotoCaptureComponent::ClientReceivePhotoResult_Implementation(
	const FNPPhotoEvidenceResult& Result)
{
	UE_LOG(
		LogNPPhoto,
		Log,
		TEXT("[Result] Success=%s Reason=%d Thief=%s Relic=%s"),
		Result.bSuccess ? TEXT("true") : TEXT("false"),
		static_cast<int32>(Result.FailureReason),
		*GetNameSafe(Result.Thief),
		*GetNameSafe(Result.Relic));

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (Result.bSuccess && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Green,
			FString::Printf(
				TEXT("[사진 판정 성공]\n촬영자: %s\n도둑: %s\n유물: %s"),
				*GetNameSafe(Result.Photographer),
				*GetNameSafe(Result.Thief),
				*GetNameSafe(Result.Relic)));
	}
#endif

	if (TArray<uint8>* JpegData = PendingJpegPhotos.Find(Result.CaptureSequence))
	{
		if (Result.bSuccess
			|| Result.FailureReason == ENPPhotoEvidenceFailureReason::NoValidEvidence)
		{
			if (UNPPhotoTransferComponent* TransferComponent =
				GetOwner()->FindComponentByClass<UNPPhotoTransferComponent>())
			{
				TransferComponent->BeginUploadPhoto(
					Result.CaptureSequence,
					*JpegData,
					CaptureWidth,
					CaptureHeight);
			}
			else
			{
				UE_LOG(LogNPPhoto, Error, TEXT("[PhotoTransfer] Transfer Component is missing."));
			}
		}
		PendingJpegPhotos.Remove(Result.CaptureSequence);
	}
	OnPhotoResultReceived.Broadcast(Result);
}
