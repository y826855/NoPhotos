#include "NPRoomGameMode.h"

#include "Core/Component/NPRoomPlayerComponent.h"
#include "Core/Room/NPRoomPlayerController.h"
#include "Core/NPPlayerState.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "NPRoomGameState.h"
#include "NPRoomLog.h"
#include "NPRoomSubsystem.h"
#include "TimerManager.h"

ANPRoomGameMode::ANPRoomGameMode()
{
	GameStateClass = ANPRoomGameState::StaticClass();
	PlayerControllerClass = ANPRoomPlayerController::StaticClass();
	PlayerStateClass = ANPPlayerState::StaticClass();
	bUseSeamlessTravel = true;
}

void ANPRoomGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!TryActivateExistingWaitingRoom())
	{
		return;
	}

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		RestoreReturningPlayer(Iterator->Get());
	}
}

void ANPRoomGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ANPRoomPlayerController* NPPC = Cast<ANPRoomPlayerController>(NewPlayer);
	if (!IsValid(NPPC))
	{
		return;
	}
	
	if (!bRoomActive)
	{
		TryActivateExistingWaitingRoom();
	}

	if (!bRoomActive)
	{
		NPPC->ClientShowMainMenuUI();
		return;
	}

	if (!IsValid(NewPlayer) || !IsValid(NewPlayer->PlayerState))
	{
		NPRoomLog::Warning(this, TEXT("플레이어 입장 처리 실패: PlayerController 또는 PlayerState가 유효하지 않습니다."));
		return;
	}

	if (ANPRoomGameState* RoomGameState = GetGameState<ANPRoomGameState>())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UNPRoomSubsystem* RoomSubsystem = GameInstance->GetSubsystem<UNPRoomSubsystem>())
			{
				RoomSubsystem->UpdateRoomPlayerCount(RoomGameState->PlayerArray.Num());
			}
		}

		NPRoomLog::Info(
			this,
			FString::Printf(
				TEXT("플레이어 입장 완료: Player=%s, Role=Guest, PlayerCount=%d"),
				*NewPlayer->PlayerState->GetPlayerName(),
				GetNumPlayers()));
		
		NPPC->ClientShowLobbyUI();
		return;
	}

	NPRoomLog::Warning(this, TEXT("플레이어 입장 처리 실패: ANPRoomGameState를 찾지 못했습니다."));
}

void ANPRoomGameMode::Logout(AController* Exiting)
{
	if (!bRoomActive)
	{
		Super::Logout(Exiting);
		return;
	}

	APlayerState* ExitingPlayerState = Exiting ? Exiting->PlayerState : nullptr;
	ANPRoomGameState* RoomGameState = GetGameState<ANPRoomGameState>();
	const bool bExitingPlayerIsHost = RoomGameState && RoomGameState->IsRoomHost(ExitingPlayerState);
	NPRoomLog::Info(
		this,
		FString::Printf(TEXT("플레이어 퇴장: Player=%s"), ExitingPlayerState ? *ExitingPlayerState->GetPlayerName() : TEXT("Unknown")));

	if (bExitingPlayerIsHost)
	{
		RoomGameState->SetHostPlayerState(nullptr);
		bRoomActive = false;
	}

	Super::Logout(Exiting);

	if (!bExitingPlayerIsHost && RoomGameState)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UNPRoomSubsystem* RoomSubsystem = GameInstance->GetSubsystem<UNPRoomSubsystem>())
			{
				RoomSubsystem->UpdateRoomPlayerCount(RoomGameState->PlayerArray.Num());
			}
		}
	}
}

void ANPRoomGameMode::HandleSeamlessTravelPlayer(AController*& Controller)
{
	Super::HandleSeamlessTravelPlayer(Controller);

	if (!TryActivateExistingWaitingRoom())
	{
		return;
	}

	RestoreReturningPlayer(Controller);

	if (ANPRoomPlayerController* NPPlayerController = Cast<ANPRoomPlayerController>(Controller))
	{
		NPPlayerController->ClientShowLobbyUI();
	}
}

bool ANPRoomGameMode::ActivateRoom(APlayerController* HostPlayer)
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

	ANPRoomGameState* RoomGameState = GetGameState<ANPRoomGameState>();
	if (!RoomGameState)
	{
		NPRoomLog::Warning(this, TEXT("방 활성화 실패: ANPRoomGameState를 찾지 못했습니다."));
		return false;
	}

	bRoomActive = true;
	RoomGameState->SetHostPlayerState(HostPlayer->PlayerState);
	NPRoomLog::Info(this, FString::Printf(TEXT("방 활성화 및 호스트 지정: Player=%s"), *HostPlayer->PlayerState->GetPlayerName()));
	return true;
}

void ANPRoomGameMode::TryStartGame(APlayerController* RequestingPlayer)
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

	const ANPRoomGameState* RoomGameState = GetGameState<ANPRoomGameState>();
	if (!RoomGameState)
	{
		NPRoomLog::Warning(this, TEXT("게임 시작 거절: ANPRoomGameState를 찾지 못했습니다."));
		return;
	}

	if (!RoomGameState->IsRoomHost(RequestingPlayer->PlayerState))
	{
		NPRoomLog::Warning(
			this,
			FString::Printf(TEXT("게임 시작 거절: 호스트가 아닙니다. Player=%s"), *RequestingPlayer->PlayerState->GetPlayerName()));
		return;
	}

	if (!RoomGameState->CanHostStartGame())
	{
		NPRoomLog::Warning(this, TEXT("게임 시작 거절: 참가한 게스트가 없습니다."));
		return;
	}

	if (GameLevel.IsNull())
	{
		NPRoomLog::Warning(this, TEXT("게임 시작 거절: GameLevel이 지정되지 않았습니다."));
		return;
	}

	const FString GameLevelPath = GameLevel.ToSoftObjectPath().GetLongPackageName();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UNPRoomSubsystem* RoomSubsystem = GameInstance->GetSubsystem<UNPRoomSubsystem>())
		{
			RoomSubsystem->MarkRoomInGame();
		}
	}

	NPRoomLog::Info(this, FString::Printf(TEXT("게임 시작 승인: ServerTravel -> %s"), *GameLevelPath));
	GetWorld()->ServerTravel(GameLevelPath);
}

void ANPRoomGameMode::RequestExitRoom(ANPPlayerController* RequestingPlayer)
{
	if (!IsValid(RequestingPlayer) || !IsValid(RequestingPlayer->PlayerState))
	{
		NPRoomLog::Warning(this, TEXT("호스트 방 나가기 실패: 요청 플레이어가 유효하지 않습니다."));
		return;
	}

	if (!bRoomActive)
	{
		NPRoomLog::Warning(this, TEXT("호스트 이전은 대기방에서만 지원합니다. 현재 방을 종료합니다."));
		if (UNPRoomPlayerComponent* RoomComponent = RequestingPlayer->GetRoomComponent())
		{
			RoomComponent->ClientLeaveRoom();
		}
		return;
	}

	ANPRoomGameState* RoomGameState = GetGameState<ANPRoomGameState>();
	if (!RoomGameState)
	{
		NPRoomLog::Warning(this, TEXT("호스트 이전 실패: ANPRoomGameState를 찾지 못했습니다."));
		return;
	}

	if (!RoomGameState->IsRoomHost(RequestingPlayer->PlayerState))
	{
		NPRoomLog::Warning(this, TEXT("호스트 이전 실패: 요청 플레이어가 현재 호스트가 아닙니다."));
		return;
	}

	APlayerState* NextHostPlayerState = nullptr;
	for (APlayerState* PlayerState : RoomGameState->PlayerArray)
	{
		if (PlayerState != RequestingPlayer->PlayerState && IsValid(PlayerState))
		{
			NextHostPlayerState = PlayerState;
			break;
		}
	}

	ANPPlayerController* NextHostController = nullptr;
	if (NextHostPlayerState)
	{
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			ANPPlayerController* PlayerController = Cast<ANPPlayerController>(Iterator->Get());
			if (PlayerController && PlayerController->PlayerState == NextHostPlayerState)
			{
				NextHostController = PlayerController;
				break;
			}
		}
	}

	bRoomActive = false;
	RoomGameState->SetHostPlayerState(nullptr);

	if (!NextHostController)
	{
		NPRoomLog::Info(this, TEXT("마지막 참가자인 호스트가 퇴장합니다. Steam 방을 삭제합니다."));
		if (UNPRoomPlayerComponent* RoomComponent = RequestingPlayer->GetRoomComponent())
		{
			RoomComponent->ClientLeaveRoom();
		}
		return;
	}

	const FString MigrationId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		ANPPlayerController* PlayerController = Cast<ANPPlayerController>(Iterator->Get());
		if (!PlayerController || PlayerController == RequestingPlayer)
		{
			continue;
		}

		if (UNPRoomPlayerComponent* RoomComponent = PlayerController->GetRoomComponent())
		{
			RoomComponent->ClientBeginHostMigration(MigrationId, PlayerController == NextHostController);
		}
	}

	NPRoomLog::Info(
		this,
		FString::Printf(TEXT("대기방 호스트 이전 시작: NextHost=%s"), *NextHostPlayerState->GetPlayerName()));

	PendingExitingHost = RequestingPlayer;
	GetWorldTimerManager().SetTimer(
		HostMigrationExitTimer,
		this,
		&ANPRoomGameMode::FinishHostMigrationExit,
		1.0f,
		false);
}

void ANPRoomGameMode::FinishHostMigrationExit()
{
	if (IsValid(PendingExitingHost))
	{
		if (UNPRoomPlayerComponent* RoomComponent = PendingExitingHost->GetRoomComponent())
		{
			RoomComponent->ClientLeaveRoom();
		}
	}

	PendingExitingHost = nullptr;
}

bool ANPRoomGameMode::TryActivateExistingWaitingRoom()
{
	if (bRoomActive)
	{
		return true;
	}
	if (GetNetMode() != NM_ListenServer)
	{
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem || !RoomSubsystem->IsWaitingRoomActive())
	{
		return false;
	}

	bRoomActive = true;
	NPRoomLog::Info(this, TEXT("기존 온라인 방 복구 완료: 대기방 상태를 다시 활성화합니다."));
	return true;
}

void ANPRoomGameMode::RestoreReturningPlayer(AController* Controller)
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController || !PlayerController->PlayerState)
	{
		return;
	}

	if (ANPPlayerState* NPPlayerState = Cast<ANPPlayerState>(PlayerController->PlayerState))
	{
		NPPlayerState->ResetPlayerScore();
	}

	ANPRoomGameState* RoomGameState = GetGameState<ANPRoomGameState>();
	if (!RoomGameState)
	{
		return;
	}

	if (PlayerController->IsLocalController())
	{
		RoomGameState->SetHostPlayerState(PlayerController->PlayerState);
	}
}
