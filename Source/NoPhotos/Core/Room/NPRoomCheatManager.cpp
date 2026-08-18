#include "NPRoomCheatManager.h"

#include "NPRoomLog.h"
#include "Core/NPPlayerController.h"

void UNPRoomCheatManager::Create()
{
	if (ANPPlayerController* NPPlayerController = Cast<ANPPlayerController>(GetPlayerController()))
	{
		NPPlayerController->HostRoom();
		return;
	}

	NPRoomLog::Warning(this, TEXT("create 실패: ANPPlayerController를 찾지 못했습니다."));
}

void UNPRoomCheatManager::List()
{
	if (ANPPlayerController* NPPlayerController = Cast<ANPPlayerController>(GetPlayerController()))
	{
		NPPlayerController->FindRooms();
		return;
	}

	NPRoomLog::Warning(this, TEXT("list 실패: ANPPlayerController를 찾지 못했습니다."));
}

void UNPRoomCheatManager::Join(const int32 RoomNumber)
{
	if (ANPPlayerController* NPPlayerController = Cast<ANPPlayerController>(GetPlayerController()))
	{
		NPPlayerController->JoinRoom(RoomNumber);
		return;
	}

	NPRoomLog::Warning(this, TEXT("join 실패: ANPPlayerController를 찾지 못했습니다."));
}

void UNPRoomCheatManager::Ready()
{
	ANPPlayerController* NPPlayerController = Cast<ANPPlayerController>(GetPlayerController());
	if (!NPPlayerController)
	{
		NPRoomLog::Warning(this, TEXT("ready 실패: ANPPlayerController를 찾지 못했습니다."));
		return;
	}

	if (NPPlayerController->IsRoomHost())
	{
		NPRoomLog::Warning(NPPlayerController, TEXT("호스트는 ready 명령어를 사용할 수 없습니다."));
		return;
	}

	NPPlayerController->SetReady(true);
}

void UNPRoomCheatManager::Start()
{
	ANPPlayerController* NPPlayerController = Cast<ANPPlayerController>(GetPlayerController());
	if (!NPPlayerController)
	{
		NPRoomLog::Warning(this, TEXT("start 실패: ANPPlayerController를 찾지 못했습니다."));
		return;
	}

	if (!NPPlayerController->IsRoomHost())
	{
		NPRoomLog::Warning(NPPlayerController, TEXT("게스트는 start 명령어를 사용할 수 없습니다."));
		return;
	}

	NPPlayerController->RequestStartGame();
}

void UNPRoomCheatManager::Out(const FString& Command)
{
	if (!Command.Equals(TEXT("game"), ESearchCase::IgnoreCase))
	{
		NPRoomLog::Warning(this, TEXT("out 실패: out game 형식으로 입력해 주세요."));
		return;
	}

	if (ANPPlayerController* NPPlayerController = Cast<ANPPlayerController>(GetPlayerController()))
	{
		NPPlayerController->ExitRoom();
		return;
	}

	NPRoomLog::Warning(this, TEXT("out game 실패: ANPPlayerController를 찾지 못했습니다."));
}
