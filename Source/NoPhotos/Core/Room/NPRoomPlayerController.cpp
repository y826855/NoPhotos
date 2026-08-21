// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Room/NPRoomPlayerController.h"

#include "Engine/GameInstance.h"
#include "SubSystem/NPUIManagerSubsystem.h"
#include "UI/NPUserWidget.h"

void ANPRoomPlayerController::ShowMainMenuUI()
{
	ShowSingleScreen(MainMenuWidgetClass);
}

void ANPRoomPlayerController::ClientShowMainMenuUI_Implementation()
{
	ShowMainMenuUI();
}

void ANPRoomPlayerController::ShowLobbyUI()
{
	ShowSingleScreen(LobbyWidgetClass);
}

void ANPRoomPlayerController::ClientShowLobbyUI_Implementation()
{
	ShowLobbyUI();
}

void ANPRoomPlayerController::ShowSingleScreen(TSubclassOf<UNPUserWidget> WidgetClass)
{
	if (!IsLocalController() || !IsValid(WidgetClass))
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UNPUIManagerSubsystem* UIManager = GameInstance
		? GameInstance->GetSubsystem<UNPUIManagerSubsystem>()
		: nullptr;
	if (!UIManager)
	{
		return;
	}

	UIManager->PopAllWidgets();
	UIManager->PushWidget(WidgetClass);
}

