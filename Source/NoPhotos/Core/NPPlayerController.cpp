#include "NPPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Core/Component/NPRoomPlayerComponent.h"
#include "Core/Room/NPRoomCheatManager.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Widgets/Input/SVirtualJoystick.h"

#include "NPGameMode.h"
#include "NPGameState.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "Room/NPRoomCheatManager.h"
#include "Room/NPRoomLog.h"
#include "Room/NPRoomSubsystem.h"
#include "SubSystem/NPUIManagerSubsystem.h"
#include "UI/NPUserWidget.h"

ANPPlayerController::ANPPlayerController()
{
	RoomComponent = CreateDefaultSubobject<UNPRoomPlayerComponent>(TEXT("RoomComponent"));
	CheatClass = UNPRoomCheatManager::StaticClass();
}

UNPRoomPlayerComponent* ANPPlayerController::GetRoomComponent() const
{
	return RoomComponent;
}

bool ANPPlayerController::HostRoom()
{
	return RoomComponent && RoomComponent->HostRoom();
}

bool ANPPlayerController::FindRooms()
{
	return RoomComponent && RoomComponent->FindRooms();
}

bool ANPPlayerController::JoinRoom(const int32 RoomNumber)
{
	return RoomComponent && RoomComponent->JoinRoom(RoomNumber);
}

void ANPPlayerController::RequestStartGame()
{
	if (RoomComponent)
	{
		RoomComponent->RequestStartGame();
	}
}

void ANPPlayerController::RequestRestartRoom()
{
	if (RoomComponent)
	{
		RoomComponent->RequestRestartRoom();
	}
}

void ANPPlayerController::ExitRoom()
{
	if (RoomComponent)
	{
		RoomComponent->ExitRoom();
	}
}

void ANPPlayerController::ShowRoomUsers() const
{
	if (RoomComponent)
	{
		RoomComponent->ShowRoomUsers();
	}
}

bool ANPPlayerController::IsRoomHost() const
{
	return RoomComponent && RoomComponent->IsRoomHost();
}

bool ANPPlayerController::CanStartGame() const
{
	return RoomComponent && RoomComponent->CanStartGame();
}

void ANPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!ShouldUseTouchControls() || !IsLocalPlayerController() || !MobileControlsWidgetClass)
	{
		return;
	}

	MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
	if (MobileControlsWidget)
	{
		MobileControlsWidget->AddToPlayerScreen(0);
	}
}

void ANPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!InputSubsystem)
	{
		return;
	}

	for (UInputMappingContext* MappingContext : DefaultMappingContexts)
	{
		if (MappingContext)
		{
			InputSubsystem->AddMappingContext(MappingContext, 0);
		}
	}

	if (ShouldUseTouchControls())
	{
		return;
	}

	for (UInputMappingContext* MappingContext : MobileExcludedMappingContexts)
	{
		if (MappingContext)
		{
			InputSubsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

bool ANPPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

#pragma region UI
void ANPPlayerController::ShowMainMenuUI()
{
	ShowSingleScreen(MainMenuWidgetClass);
}

void ANPPlayerController::ShowLobbyUI()
{
	ShowSingleScreen(LobbyWidgetClass);
}

void ANPPlayerController::ClientShowLobbyUI_Implementation()
{
	ShowLobbyUI();
}

void ANPPlayerController::ShowGameScreenUI()
{
	ShowSingleScreen(GameScreenWidgetClass);
}

void ANPPlayerController::ClientShowGameScreenUI_Implementation()
{
	ShowGameScreenUI();
}

void ANPPlayerController::ShowSingleScreen(TSubclassOf<UNPUserWidget> WidgetClass)
{
	if (!IsLocalController() || !IsValid(WidgetClass))
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}

	UNPUIManagerSubsystem* UIManager =
		GameInstance->GetSubsystem<UNPUIManagerSubsystem>();
	if (!IsValid(UIManager))
	{
		return;
	}

	UIManager->PopAllWidgets();
	UIManager->PushWidget(WidgetClass);
}
#pragma endregion
