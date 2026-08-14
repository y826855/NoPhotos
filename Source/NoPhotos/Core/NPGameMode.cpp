// Fill out your copyright notice in the Description page of Project Settings.


#include "NPGameMode.h"

#include "NPGameState.h"
#include "NPPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Room/NPRoomLog.h"

ANPGameMode::ANPGameMode()
{
	GameStateClass = ANPGameState::StaticClass();
	PlayerControllerClass = ANPPlayerController::StaticClass();
	bUseSeamlessTravel = true;
}

void ANPGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!bRoomActive)
	{
		return;
	}

	if (!IsValid(NewPlayer) || !IsValid(NewPlayer->PlayerState))
	{
		NPRoomLog::Warning(this, TEXT("플레이어 입장 처리 실패: PlayerController 또는 PlayerState가 유효하지 않습니다."));
		return;
	}

	if (ANPGameState* NPGameState = GetGameState<ANPGameState>())
	{
		NPGameState->AddRoomMember(NewPlayer->PlayerState, false);
		NPRoomLog::Info(
			this,
			FString::Printf(
				TEXT("플레이어 입장 완료: Player=%s, Role=Guest, PlayerCount=%d"),
				*NewPlayer->PlayerState->GetPlayerName(),
				GetNumPlayers()));
		return;
	}

	NPRoomLog::Warning(this, TEXT("플레이어 입장 처리 실패: ANPGameState를 찾지 못했습니다."));
}

void ANPGameMode::Logout(AController* Exiting)
{
	if (!bRoomActive)
	{
		Super::Logout(Exiting);
		return;
	}

	APlayerState* ExitingPlayerState = Exiting ? Exiting->PlayerState : nullptr;
	NPRoomLog::Info(
		this,
		FString::Printf(TEXT("플레이어 퇴장: Player=%s"), ExitingPlayerState ? *ExitingPlayerState->GetPlayerName() : TEXT("Unknown")));

	if (ANPGameState* NPGameState = GetGameState<ANPGameState>())
	{
		NPGameState->RemoveRoomMember(ExitingPlayerState);
	}

	if (ExitingPlayerState == HostPlayerState)
	{
		HostPlayerState = nullptr;
		bRoomActive = false;
	}

	Super::Logout(Exiting);
}

bool ANPGameMode::ActivateRoom(APlayerController* HostPlayer)
{
	if (bRoomActive)
	{
		NPRoomLog::Warning(this, TEXT("방 활성화 실패: 이미 활성화된 방입니다."));
		return false;
	}

	if (!IsValid(HostPlayer) || !IsValid(HostPlayer->PlayerState))
	{
		NPRoomLog::Warning(this, TEXT("방 활성화 실패: 호스트 PlayerController 또는 PlayerState가 유효하지 않습니다."));
		return false;
	}

	ANPGameState* NPGameState = GetGameState<ANPGameState>();
	if (!NPGameState)
	{
		NPRoomLog::Warning(this, TEXT("방 활성화 실패: ANPGameState를 찾지 못했습니다."));
		return false;
	}

	bRoomActive = true;
	HostPlayerState = HostPlayer->PlayerState;
	NPGameState->AddRoomMember(HostPlayerState, true);
	NPRoomLog::Info(this, FString::Printf(TEXT("방 활성화 및 호스트 지정: Player=%s"), *HostPlayerState->GetPlayerName()));
	return true;
}

void ANPGameMode::SetPlayerReady(APlayerController* PlayerController, const bool bIsReady)
{
	if (!bRoomActive)
	{
		NPRoomLog::Warning(this, TEXT("준비 처리 실패: 아직 방이 활성화되지 않았습니다."));
		return;
	}

	if (!IsValid(PlayerController) || !IsValid(PlayerController->PlayerState))
	{
		NPRoomLog::Warning(this, TEXT("준비 처리 실패: PlayerController 또는 PlayerState가 유효하지 않습니다."));
		return;
	}

	if (ANPGameState* NPGameState = GetGameState<ANPGameState>())
	{
		NPGameState->SetRoomMemberReady(PlayerController->PlayerState, bIsReady);
		return;
	}

	NPRoomLog::Warning(this, TEXT("준비 처리 실패: ANPGameState를 찾지 못했습니다."));
}

void ANPGameMode::TryStartGame(APlayerController* RequestingPlayer)
{
	if (!bRoomActive)
	{
		NPRoomLog::Warning(this, TEXT("게임 시작 거절: 아직 방이 활성화되지 않았습니다."));
		return;
	}

	if (!IsValid(RequestingPlayer) || !IsValid(RequestingPlayer->PlayerState))
	{
		NPRoomLog::Warning(this, TEXT("게임 시작 거절: 요청 PlayerController 또는 PlayerState가 유효하지 않습니다."));
		return;
	}

	if (RequestingPlayer->PlayerState != HostPlayerState)
	{
		NPRoomLog::Warning(
			this,
			FString::Printf(TEXT("게임 시작 거절: 호스트가 아닙니다. Player=%s"), *RequestingPlayer->PlayerState->GetPlayerName()));
		return;
	}

	const ANPGameState* NPGameState = GetGameState<ANPGameState>();
	if (!NPGameState)
	{
		NPRoomLog::Warning(this, TEXT("게임 시작 거절: ANPGameState를 찾지 못했습니다."));
		return;
	}

	if (!NPGameState->CanHostStartGame())
	{
		NPRoomLog::Warning(this, TEXT("게임 시작 거절: 게스트가 없거나 아직 준비하지 않은 게스트가 있습니다."));
		return;
	}

	if (GameLevel.IsNull())
	{
		NPRoomLog::Warning(this, TEXT("게임 시작 거절: GameLevel이 지정되지 않았습니다."));
		return;
	}

	const FString GameLevelPath = GameLevel.ToSoftObjectPath().GetLongPackageName();
	NPRoomLog::Info(this, FString::Printf(TEXT("게임 시작 승인: ServerTravel -> %s"), *GameLevelPath));
	GetWorld()->ServerTravel(GameLevelPath);
}
