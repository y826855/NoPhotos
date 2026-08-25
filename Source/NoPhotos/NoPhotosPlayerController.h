// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NoPhotosPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UNPPhotoCaptureComponent;
class UNPPhotoFlashWidget;
class UNPPhotoTransferComponent;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class ANoPhotosPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANoPhotosPlayerController();

	UFUNCTION(BlueprintPure, Category="Photo")
	UNPPhotoCaptureComponent* GetPhotoCaptureComponent() const { return PhotoCaptureComponent; }

	UFUNCTION(BlueprintPure, Category="Photo")
	UNPPhotoTransferComponent* GetPhotoTransferComponent() const { return PhotoTransferComponent; }

	/** 로컬 사진 플래시 UI 애니메이션을 실행합니다. */
	void PlayPhotoFlash();

protected:
	/** 마우스 우클릭에 매핑할 사진 모드 진입/해제 액션입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Photo")
	TObjectPtr<UInputAction> TogglePhotoModeAction;

	/** 사진 모드에서 마우스 좌클릭에 매핑할 실제 촬영 액션입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input|Photo")
	TObjectPtr<UInputAction> TakePhotoAction;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** UNPPhotoFlashWidget을 부모로 만든 전체 화면 플래시 WBP 클래스입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Photo|UI")
	TSubclassOf<UNPPhotoFlashWidget> PhotoFlashWidgetClass;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

private:
	void HandleTogglePhotoModeInput();
	void HandleTakePhotoInput();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Photo", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNPPhotoCaptureComponent> PhotoCaptureComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Photo", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNPPhotoTransferComponent> PhotoTransferComponent;

	UPROPERTY(Transient)
	TObjectPtr<UNPPhotoFlashWidget> PhotoFlashWidget;

};
