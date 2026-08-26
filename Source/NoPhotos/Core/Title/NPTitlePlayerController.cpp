// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Title/NPTitlePlayerController.h"

#include "Core/Component/NPRoomPlayerComponent.h"
#include "Core/Room/NPRoomCheatManager.h"
#include "Engine/GameInstance.h"
#include "SubSystem/NPUIManagerSubsystem.h"
#include "UI/NPUserWidget.h"

ANPTitlePlayerController::ANPTitlePlayerController()
{
	RoomComponent = CreateDefaultSubobject<UNPRoomPlayerComponent>(TEXT("RoomComponent"));
	CheatClass = UNPRoomCheatManager::StaticClass();
}

bool ANPTitlePlayerController::HostRoom()
{
	return RoomComponent && RoomComponent->HostRoom();
}

bool ANPTitlePlayerController::FindRooms()
{
	return RoomComponent && RoomComponent->FindRooms();
}

bool ANPTitlePlayerController::JoinRoom(const int32 RoomNumber)
{
	return RoomComponent && RoomComponent->JoinRoom(RoomNumber);
}

void ANPTitlePlayerController::ShowMainMenuUI()
{
	if (!IsLocalController() || !IsValid(MainMenuWidgetClass))
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
	UIManager->PushWidget(MainMenuWidgetClass);
}

void ANPTitlePlayerController::ClientShowMainMenuUI_Implementation()
{
	ShowMainMenuUI();
}

