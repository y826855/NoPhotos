// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Main/NPMainPlayerController.h"

#include "Engine/GameInstance.h"
#include "SubSystem/NPUIManagerSubsystem.h"
#include "UI/NPUserWidget.h"

void ANPMainPlayerController::ShowGameScreenUI()
{
	ShowSingleScreen(GameScreenWidgetClass);
}

void ANPMainPlayerController::ClientShowGameScreenUI_Implementation()
{
	ShowGameScreenUI();
}

void ANPMainPlayerController::ShowSelectPictureUI()
{
	ShowSingleScreen(SelectPictureWidgetClass);
}

void ANPMainPlayerController::ClientShowSelectPictureUI_Implementation()
{
	ShowSelectPictureUI();
}

void ANPMainPlayerController::ShowResultUI()
{
	ShowSingleScreen(ResultWidgetClass);
}

void ANPMainPlayerController::ClientShowResultUI_Implementation()
{
	ShowResultUI();
}

void ANPMainPlayerController::ShowSingleScreen(
	TSubclassOf<UNPUserWidget> WidgetClass)
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

