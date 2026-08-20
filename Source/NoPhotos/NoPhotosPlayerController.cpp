// Copyright Epic Games, Inc. All Rights Reserved.


#include "NoPhotosPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "NoPhotos.h"
#include "Gameplay/Photo/NPPhotoCaptureComponent.h"
#include "Gameplay/Photo/NPPhotoLog.h"
#include "Gameplay/Photo/NPPhotoPreviewWidget.h"
#include "Gameplay/Photo/NPPhotoTransferComponent.h"
#include "Widgets/Input/SVirtualJoystick.h"

ANoPhotosPlayerController::ANoPhotosPlayerController()
{
	PhotoCaptureComponent = CreateDefaultSubobject<UNPPhotoCaptureComponent>(TEXT("PhotoCaptureComponent"));
	PhotoTransferComponent = CreateDefaultSubobject<UNPPhotoTransferComponent>(TEXT("PhotoTransferComponent"));
}

void ANoPhotosPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		if (PhotoPreviewWidgetClass)
		{
			PhotoPreviewWidget = CreateWidget<UNPPhotoPreviewWidget>(this, PhotoPreviewWidgetClass);
			if (PhotoPreviewWidget)
			{
				PhotoPreviewWidget->AddToPlayerScreen();
				PhotoPreviewWidget->HidePhoto();
				UE_LOG(LogNPPhoto, Log, TEXT("[PhotoUI] Preview widget created. Widget=%s"), *GetNameSafe(PhotoPreviewWidget));
			}
		}
		else
		{
			UE_LOG(LogNPPhoto, Warning, TEXT("[PhotoUI] PhotoPreviewWidgetClass is not assigned."));
		}
	}

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogNoPhotos, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ANoPhotosPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}

	UEnhancedInputComponent* EnhancedInputComponent =
		Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent || !TakePhotoAction)
	{
		UE_LOG(
			LogNPPhoto,
			Warning,
			TEXT("[Input] Photo binding skipped. EnhancedInput=%s TakePhotoAction=%s"),
			*GetNameSafe(EnhancedInputComponent),
			*GetNameSafe(TakePhotoAction));
		return;
	}

	EnhancedInputComponent->BindAction(
		TakePhotoAction,
		ETriggerEvent::Started,
		this,
		&ANoPhotosPlayerController::HandleTakePhotoInput);
	UE_LOG(LogNPPhoto, Log, TEXT("[Input] Photo action bound. Action=%s"), *GetNameSafe(TakePhotoAction));
}

void ANoPhotosPlayerController::HandleTakePhotoInput()
{
	UE_LOG(
		LogNPPhoto,
		Log,
		TEXT("[Input] Photo input received. Controller=%s Local=%s"),
		*GetNameSafe(this),
		IsLocalController() ? TEXT("true") : TEXT("false"));

	if (!PhotoCaptureComponent)
	{
		UE_LOG(LogNPPhoto, Error, TEXT("[Input] PhotoCaptureComponent is null."));
		return;
	}

	const bool bStarted = PhotoCaptureComponent->TakePhoto();
	UE_LOG(
		LogNPPhoto,
		Log,
		TEXT("[Input] TakePhoto result=%s"),
		bStarted ? TEXT("success") : TEXT("failed"));
}

bool ANoPhotosPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
