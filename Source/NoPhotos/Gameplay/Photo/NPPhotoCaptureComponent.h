#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gameplay/Photo/NPPhotoEvidenceTypes.h"
#include "NPPhotoCaptureComponent.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class ANPStablePhysicsPawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FNPOnPhotoResultReceived,
	FNPPhotoEvidenceResult,
	Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FNPOnPhotoCaptured,
	UTextureRenderTarget2D*,
	Photo);

/** 로컬 사진 이미지를 만들고 소유 PlayerController에서 서버 판정을 요청합니다. */
UCLASS(ClassGroup=(Photo), meta=(BlueprintSpawnableComponent))
class NOPHOTOS_API UNPPhotoCaptureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPPhotoCaptureComponent();

	UFUNCTION(BlueprintCallable, Category="Photo")
	bool TakePhoto();

	UFUNCTION(BlueprintPure, Category="Photo")
	UTextureRenderTarget2D* GetPhotoRenderTarget() const { return PhotoRenderTarget; }

	UPROPERTY(BlueprintAssignable, Category="Photo")
	FNPOnPhotoResultReceived OnPhotoResultReceived;

	/** 로컬 Scene Capture가 끝난 직후 촬영된 Render Target을 전달합니다. */
	UPROPERTY(BlueprintAssignable, Category="Photo")
	FNPOnPhotoCaptured OnPhotoCaptured;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Photo|Capture", meta=(ClampMin="64"))
	int32 CaptureWidth = 1024;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Photo|Capture", meta=(ClampMin="64"))
	int32 CaptureHeight = 576;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Photo|Capture", meta=(ClampMin="1.0", ClampMax="179.0"))
	float CaptureFOV = 60.0f;

	/** 촬영 직후 다음 촬영까지 기다려야 하는 시간입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Photo|Rules", meta=(ClampMin="0.0"))
	float PhotoCooldown = 5.0f;

	/** 셔터를 누른 뒤 이동 입력을 잠그는 시간입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Photo|Rules", meta=(ClampMin="0.0"))
	float MovementLockDuration = 0.5f;

private:
	void InitializeLocalCapture();
	bool IsPhotographerGrabbing() const;
	void CancelPhotoAttempt();
	void ApplyMovementLock();
	void ReleaseMovementLock();

	UFUNCTION(Server, Reliable)
	void ServerRequestTakePhoto(
		FVector_NetQuantize10 CameraLocation,
		FVector_NetQuantizeNormal CameraForward,
		uint16 CaptureSequence);

	UFUNCTION(Client, Reliable)
	void ClientReceivePhotoResult(const FNPPhotoEvidenceResult& Result);

	UPROPERTY(Transient)
	TObjectPtr<USceneCaptureComponent2D> SceneCapture = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> PhotoRenderTarget = nullptr;

	double LastLocalCaptureTime = -TNumericLimits<double>::Max();
	double LastServerCaptureTime = -TNumericLimits<double>::Max();
	uint16 NextCaptureSequence = 0;
	bool bPhotoAttemptInProgress = false;
	bool bMovementLockApplied = false;
	TWeakObjectPtr<ANPStablePhysicsPawn> MovementLockedPawn;
	FTimerHandle MovementUnlockTimer;
};
