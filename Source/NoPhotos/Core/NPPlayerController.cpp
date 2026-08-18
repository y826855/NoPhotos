// Fill out your copyright notice in the Description page of Project Settings.


#include "NPPlayerController.h"

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
	CheatClass = UNPRoomCheatManager::StaticClass();
}

void ANPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UNPRoomSubsystem* RoomSubsystem = GameInstance->GetSubsystem<UNPRoomSubsystem>())
			{
				RoomSubsystem->LogOnlineServiceStatus();
			}
		}

		EnableCheats();
		NPRoomLog::Info(this, TEXT("방 테스트 명령어 활성화: create, list, join {방번호}, ready, start, out game"));
		
		if (IsLocalController() && GetNetMode() == NM_Standalone)
        {
            ShowMainMenuUI();
        }
	}
}

bool ANPPlayerController::HostRoom()
{
	if (!IsLocalController())
	{
		NPRoomLog::Warning(this, TEXT("호스트 생성 실패: 로컬 PlayerController가 아닙니다."));
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem)
	{
		NPRoomLog::Warning(this, TEXT("호스트 생성 실패: RoomSubsystem을 찾지 못했습니다."));
		return false;
	}

	bool bSuccess = RoomSubsystem->HostRoom();
	if (bSuccess)
	{
		ShowLobbyUI();
	}

	return bSuccess;
}

bool ANPPlayerController::FindRooms()
{
	if (!IsLocalController())
	{
		NPRoomLog::Warning(this, TEXT("방 목록 검색 실패: 로컬 PlayerController가 아닙니다."));
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem)
	{
		NPRoomLog::Warning(this, TEXT("방 목록 검색 실패: RoomSubsystem을 찾지 못했습니다."));
		return false;
	}

	return RoomSubsystem->FindRooms();
}

bool ANPPlayerController::JoinRoom(const int32 RoomNumber)
{
	if (!IsLocalController())
	{
		NPRoomLog::Warning(this, TEXT("방 참가 실패: 로컬 PlayerController가 아닙니다."));
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem)
	{
		NPRoomLog::Warning(this, TEXT("방 참가 실패: RoomSubsystem을 찾지 못했습니다."));
		return false;
	}

	return RoomSubsystem->JoinRoom(RoomNumber);
}

void ANPPlayerController::SetReady(const bool bIsReady)
{
	NPRoomLog::Info(
		this,
		FString::Printf(TEXT("준비 버튼 요청: Player=%s, Ready=%s"), *GetNameSafe(PlayerState.Get()), bIsReady ? TEXT("true") : TEXT("false")));
	ServerSetReady(bIsReady);
}

void ANPPlayerController::RequestStartGame()
{
	NPRoomLog::Info(this, FString::Printf(TEXT("게임 시작 버튼 요청: Player=%s"), *GetNameSafe(PlayerState.Get())));
	ServerRequestStartGame();
}

void ANPPlayerController::ExitRoom()
{
	if (!IsLocalController())
	{
		NPRoomLog::Warning(this, TEXT("방 나가기 실패: 로컬 PlayerController가 아닙니다."));
		return;
	}

	if (IsRoomHost())
	{
		NPRoomLog::Info(this, TEXT("호스트 방 나가기 요청"));
		ServerRequestExitRoom();
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem)
	{
		NPRoomLog::Warning(this, TEXT("방 나가기 실패: RoomSubsystem을 찾지 못했습니다."));
		return;
	}

	RoomSubsystem->LeaveRoom();
}

bool ANPPlayerController::IsRoomHost() const
{
	const ANPGameState* NPGameState = GetWorld() ? GetWorld()->GetGameState<ANPGameState>() : nullptr;
	return NPGameState && NPGameState->IsRoomHost(PlayerState);
}

bool ANPPlayerController::IsRoomReady() const
{
	const ANPGameState* NPGameState = GetWorld() ? GetWorld()->GetGameState<ANPGameState>() : nullptr;
	return NPGameState && NPGameState->IsRoomMemberReady(PlayerState);
}

bool ANPPlayerController::CanStartGame() const
{
	const ANPGameState* NPGameState = GetWorld() ? GetWorld()->GetGameState<ANPGameState>() : nullptr;
	return IsRoomHost() && NPGameState && NPGameState->CanHostStartGame();
}

void ANPPlayerController::ServerSetReady_Implementation(const bool bIsReady)
{
	NPRoomLog::Info(
		this,
		FString::Printf(TEXT("준비 Server RPC 도착: Player=%s, Ready=%s"), *GetNameSafe(PlayerState.Get()), bIsReady ? TEXT("true") : TEXT("false")));

	if (ANPGameMode* NPGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ANPGameMode>() : nullptr)
	{
		NPGameMode->SetPlayerReady(this, bIsReady);
		return;
	}

	NPRoomLog::Warning(this, TEXT("준비 처리 실패: ANPGameMode를 찾지 못했습니다."));
}

void ANPPlayerController::ServerRequestStartGame_Implementation()
{
	NPRoomLog::Info(this, FString::Printf(TEXT("게임 시작 Server RPC 도착: Player=%s"), *GetNameSafe(PlayerState.Get())));

	if (ANPGameMode* NPGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ANPGameMode>() : nullptr)
	{
		NPGameMode->TryStartGame(this);
		return;
	}

	NPRoomLog::Warning(this, TEXT("게임 시작 처리 실패: ANPGameMode를 찾지 못했습니다."));
}

void ANPPlayerController::ServerRequestExitRoom_Implementation()
{
	if (ANPGameMode* NPGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ANPGameMode>() : nullptr)
	{
		NPGameMode->RequestExitRoom(this);
		return;
	}

	NPRoomLog::Warning(this, TEXT("호스트 방 나가기 실패: ANPGameMode를 찾지 못했습니다."));
}

void ANPPlayerController::ClientBeginHostMigration_Implementation(
	const FString& MigrationId,
	const bool bBecomeHost)
{
	UGameInstance* GameInstance = GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem)
	{
		NPRoomLog::Warning(this, TEXT("호스트 이전 실패: RoomSubsystem을 찾지 못했습니다."));
		return;
	}

	RoomSubsystem->BeginHostMigration(MigrationId, bBecomeHost);
}

void ANPPlayerController::ClientLeaveRoom_Implementation()
{
	UGameInstance* GameInstance = GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem)
	{
		NPRoomLog::Warning(this, TEXT("방 나가기 실패: RoomSubsystem을 찾지 못했습니다."));
		return;
	}

	RoomSubsystem->LeaveRoom();
}

#pragma region UI
void ANPPlayerController::ShowMainMenuUI()
{
	if (!IsLocalController()) return;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UNPUIManagerSubsystem* UIManager = GI->GetSubsystem<UNPUIManagerSubsystem>())
		{
			UIManager->PopAllWidgets();
			if (IsValid(MainMenuWidgetClass))
			{
				UIManager->PushWidget(MainMenuWidgetClass);
			}
		}
	}
	
}

void ANPPlayerController::ShowLobbyUI()
{
	if (!IsLocalController()) return;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UNPUIManagerSubsystem* UIManager = GI->GetSubsystem<UNPUIManagerSubsystem>())
		{
			UIManager->PopAllWidgets();
			if (IsValid(LobbyWidgetClass))
			{
				UIManager->PushWidget(LobbyWidgetClass);
			}
		}
	}
}

void ANPPlayerController::ClientShowLobbyUI_Implementation()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}

	UNPUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UNPUIManagerSubsystem>();
	if (!IsValid(UIManager))
	{
		return;
	}

	UIManager->PopAllWidgets(); 
	if (LobbyWidgetClass)
	{
		UIManager->PushWidget(LobbyWidgetClass);
	}
}
#pragma endregion
