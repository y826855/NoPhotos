#include "Core/Main/NPMainPlayerController.h"

#include "Core/Main/NPMainGameMode.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "SubSystem/NPUIManagerSubsystem.h"
#include "UI/NPUserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Core/Room/NPRoomSubsystem.h"

ANPMainPlayerController::ANPMainPlayerController()
{
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultMapping(
		TEXT("/Game/Input/IMC_Default.IMC_Default"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MouseLookMapping(
		TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"));
	static ConstructorHelpers::FObjectFinder<UInputAction> PhotoModeAction(
		TEXT("/Game/Input/Actions/IA_PhotoMode.IA_PhotoMode"));
	static ConstructorHelpers::FObjectFinder<UInputAction> PhotoShotAction(
		TEXT("/Game/Input/Actions/IA_PhotoShot.IA_PhotoShot"));

	if (DefaultMapping.Succeeded())
	{
		DefaultMappingContexts.Add(DefaultMapping.Object);
	}
	if (MouseLookMapping.Succeeded())
	{
		DefaultMappingContexts.Add(MouseLookMapping.Object);
	}
	if (PhotoModeAction.Succeeded())
	{
		TogglePhotoModeAction = PhotoModeAction.Object;
	}
	if (PhotoShotAction.Succeeded())
	{
		TakePhotoAction = PhotoShotAction.Object;
	}
}

bool ANPMainPlayerController::IsListenServerHost() const
{
	return IsLocalController() && HasAuthority();
}

void ANPMainPlayerController::RequestRestartRoom()
{
	if (IsLocalController())
	{
		ServerRequestRestartRoom();
	}
}

void ANPMainPlayerController::ServerRequestRestartRoom_Implementation()
{
	if (ANPMainGameMode* MainGameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ANPMainGameMode>()
		: nullptr)
	{
		MainGameMode->RequestRestartRoom(this);
	}
}

void ANPMainPlayerController::ExitToMainMenu()
{
	if (!IsLocalController() || MainMenuLevel.IsNull())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GameInstance
		? GameInstance->GetSubsystem<UNPRoomSubsystem>()
		: nullptr;

	if (!RoomSubsystem)
	{
		return;
	}

	const FString MenuLevelPath = MainMenuLevel.ToSoftObjectPath().GetLongPackageName();
	RoomSubsystem->LeaveRoom(MenuLevelPath);
}

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

