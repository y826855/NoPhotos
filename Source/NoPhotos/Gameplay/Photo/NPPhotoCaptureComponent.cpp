#include "Gameplay/Photo/NPPhotoCaptureComponent.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/Character/NPStablePhysicsPawn.h"
#include "Gameplay/Character/NPStablePhysicsGrabComponent.h"
#include "Gameplay/Photo/NPPhotoLog.h"
#include "NoPhotosGameMode.h"
#include "TimerManager.h"

UNPPhotoCaptureComponent::UNPPhotoCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNPPhotoCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (PlayerController && PlayerController->IsLocalController())
	{
		InitializeLocalCapture();
	}
}

void UNPPhotoCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseMovementLock();
	Super::EndPlay(EndPlayReason);
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
	ANPStablePhysicsPawn* Pawn = Cast<ANPStablePhysicsPawn>(PlayerController->GetPawn());
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

	LastLocalCaptureTime = CurrentTime;
	bPhotoAttemptInProgress = false;
	ApplyMovementLock();
	const uint16 CaptureSequence = ++NextCaptureSequence;
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
	if (!Photographer || !GameMode)
	{
		UE_LOG(
			LogNPPhoto,
			Error,
			TEXT("[Server] Rejected: invalid photographer or GameMode. Photographer=%s GameMode=%s"),
			*GetNameSafe(Photographer),
			*GetNameSafe(GameMode));
		FNPPhotoEvidenceResult FailureResult;
		FailureResult.FailureReason = ENPPhotoEvidenceFailureReason::InvalidPhotographer;
		ClientReceivePhotoResult(FailureResult);
		return;
	}

	FNPPhotoEvidenceResult RejectedResult;
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

	FNPPhotoCaptureRequest Request;
	Request.Photographer = Photographer;
	Request.CameraLocation = CameraLocation;
	Request.CameraForward = CameraForward;
	Request.CaptureSequence = CaptureSequence;
	ClientReceivePhotoResult(GameMode->HandlePhotoCaptureRequest(Request));
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
	OnPhotoResultReceived.Broadcast(Result);
}
