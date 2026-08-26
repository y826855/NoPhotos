// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Room/NPRoomPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "SubSystem/NPUIManagerSubsystem.h"
#include "UI/NPUserWidget.h"

ANPRoomPlayerController::ANPRoomPlayerController()
	: ChangeInputAction(nullptr) {}

void ANPRoomPlayerController::ShowMainMenuUI()
{
	bIsMouseInput = false;
	SetLobbyInputMappingEnabled(false);
	ShowSingleScreen(MainMenuWidgetClass);
}

void ANPRoomPlayerController::ClientShowMainMenuUI_Implementation()
{
	ShowMainMenuUI();
}

void ANPRoomPlayerController::ShowLobbyUI()
{
	ShowSingleScreen(LobbyWidgetClass);
	bIsMouseInput = false;
	SetLobbyInputMappingEnabled(true);
	ApplyLobbyInputMode();
}

void ANPRoomPlayerController::ClientShowLobbyUI_Implementation()
{
	ShowLobbyUI();
}

void ANPRoomPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (ChangeInputAction)
		{
			EnhancedInput->BindAction(ChangeInputAction, ETriggerEvent::Started, this, &ANPRoomPlayerController::ChangeInputMode);
		}
	}
}

void ANPRoomPlayerController::ChangeInputMode()
{
	if (!IsLocalController())
	{
		return;
	}

	bIsMouseInput = !bIsMouseInput;
	ApplyLobbyInputMode();
}

// 로비 진입이나 퇴장에 맞춰 입력모드 변경
void ANPRoomPlayerController::SetLobbyInputMappingEnabled(const bool bEnabled)
{
	if (!IsLocalController() || !IsValid(InputMappingContext))
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!IsValid(InputSubsystem))
	{
		return;
	}

	//로비라면
	if (bEnabled)
	{
		InputSubsystem->AddMappingContext(InputMappingContext, 0);
		return;
	}

	InputSubsystem->RemoveMappingContext(InputMappingContext);
}

//bIsMouseInput 값을 입력 모드에 적용
void ANPRoomPlayerController::ApplyLobbyInputMode()
{
	if (!IsLocalController())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UNPUIManagerSubsystem* UIManager = GameInstance	? GameInstance->GetSubsystem<UNPUIManagerSubsystem>() : nullptr;
	UNPUserWidget* TopWidget = UIManager ? UIManager->GetTopWidget() : nullptr;
	if (!IsValid(TopWidget))
	{
		return;
	}

	TopWidget->SetInputModeState(bIsMouseInput ? ENPWidgetInputMode::GameAndUI : ENPWidgetInputMode::GameOnly);
	UIManager->RefreshTopWidgetInputMode();
}

void ANPRoomPlayerController::ShowSingleScreen(TSubclassOf<UNPUserWidget> WidgetClass)
{
	if (!IsLocalController() || !IsValid(WidgetClass))
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UNPUIManagerSubsystem* UIManager = GameInstance	? GameInstance->GetSubsystem<UNPUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		return;
	}

	UIManager->PopAllWidgets();
	UIManager->PushWidget(WidgetClass);
}
