#include "NPRoomPlayerComponent.h"

#include "Core/Main/NPMainGameMode.h"
#include "Core/NPPlayerController.h"
#include "Core/Room/NPRoomGameMode.h"
#include "Core/Room/NPRoomGameState.h"
#include "Core/Room/NPRoomLog.h"
#include "Core/Room/NPRoomSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"

UNPRoomPlayerComponent::UNPRoomPlayerComponent()
{
	SetIsReplicatedByDefault(true);
}

void UNPRoomPlayerComponent::BeginPlay()
{
	Super::BeginPlay();

	ANPPlayerController* PlayerController = GetNPPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	if (UGameInstance* GameInstance = PlayerController->GetGameInstance())
	{
		if (UNPRoomSubsystem* RoomSubsystem = GameInstance->GetSubsystem<UNPRoomSubsystem>())
		{
			RoomSubsystem->LogOnlineServiceStatus();
		}
	}

	PlayerController->EnableCheats();
	NPRoomLog::Info(this, TEXT("방 테스트 명령어 활성화: create, list, join {방번호}, user, start, rehost, out game"));
}

bool UNPRoomPlayerComponent::HostRoom()
{
	ANPPlayerController* PlayerController = GetNPPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		NPRoomLog::Warning(this, TEXT("호스트 생성 실패: 로컬 PlayerController가 아닙니다."));
		return false;
	}

	UGameInstance* GameInstance = PlayerController->GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem)
	{
		NPRoomLog::Warning(this, TEXT("호스트 생성 실패: RoomSubsystem을 찾지 못했습니다."));
		return false;
	}

	return RoomSubsystem->HostRoom();
}

bool UNPRoomPlayerComponent::FindRooms()
{
	ANPPlayerController* PlayerController = GetNPPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		NPRoomLog::Warning(this, TEXT("방 목록 검색 실패: 로컬 PlayerController가 아닙니다."));
		return false;
	}

	UGameInstance* GameInstance = PlayerController->GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem)
	{
		NPRoomLog::Warning(this, TEXT("방 목록 검색 실패: RoomSubsystem을 찾지 못했습니다."));
		return false;
	}

	return RoomSubsystem->FindRooms();
}

bool UNPRoomPlayerComponent::JoinRoom(const int32 RoomNumber)
{
	ANPPlayerController* PlayerController = GetNPPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		NPRoomLog::Warning(this, TEXT("방 참가 실패: 로컬 PlayerController가 아닙니다."));
		return false;
	}

	UGameInstance* GameInstance = PlayerController->GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem)
	{
		NPRoomLog::Warning(this, TEXT("방 참가 실패: RoomSubsystem을 찾지 못했습니다."));
		return false;
	}

	return RoomSubsystem->JoinRoom(RoomNumber);
}

void UNPRoomPlayerComponent::RequestStartGame()
{
	const ANPPlayerController* PlayerController = GetNPPlayerController();
	NPRoomLog::Info(
		this,
		FString::Printf(
			TEXT("게임 시작 버튼 요청: Player=%s"),
			PlayerController ? *GetNameSafe(PlayerController->PlayerState.Get()) : TEXT("Unknown")));
	ServerRequestStartGame();
}

void UNPRoomPlayerComponent::RequestRestartRoom()
{
	const ANPPlayerController* PlayerController = GetNPPlayerController();
	NPRoomLog::Info(
		this,
		FString::Printf(
			TEXT("대기방 복귀 버튼 요청: Player=%s"),
			PlayerController ? *GetNameSafe(PlayerController->PlayerState.Get()) : TEXT("Unknown")));
	ServerRequestRestartRoom();
}

void UNPRoomPlayerComponent::ExitRoom()
{
	ANPPlayerController* PlayerController = GetNPPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		NPRoomLog::Warning(this, TEXT("방 나가기 실패: 로컬 PlayerController가 아닙니다."));
		return;
	}

	const FString MenuLevelPath = GetMenuLevelPath();
	if (MenuLevelPath.IsEmpty())
	{
		NPRoomLog::Warning(this, TEXT("방 나가기 실패: RoomComponent의 MenuLevel이 지정되지 않았습니다."));
		return;
	}

	if (IsRoomHost())
	{
		NPRoomLog::Info(this, TEXT("호스트 방 나가기 요청"));
		ServerRequestExitRoom();
		return;
	}

	UGameInstance* GameInstance = PlayerController->GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem)
	{
		NPRoomLog::Warning(this, TEXT("방 나가기 실패: RoomSubsystem을 찾지 못했습니다."));
		return;
	}

	RoomSubsystem->LeaveRoom(MenuLevelPath);
}

void UNPRoomPlayerComponent::ShowRoomUsers() const
{
	const ANPPlayerController* PlayerController = GetNPPlayerController();
	const ANPRoomGameState* RoomGameState = PlayerController && PlayerController->GetWorld()
		? PlayerController->GetWorld()->GetGameState<ANPRoomGameState>()
		: nullptr;
	if (!RoomGameState)
	{
		NPRoomLog::Warning(this, TEXT("참가자 목록 조회 실패: ANPRoomGameState를 찾지 못했습니다."));
		return;
	}

	const TArray<FNPPlayerRoomInfo> RoomMembers = RoomGameState->GetRoomMembers();
	if (RoomMembers.IsEmpty())
	{
		NPRoomLog::Warning(this, TEXT("참가자 목록 조회 실패: 현재 참여 중인 방이 없습니다."));
		return;
	}

	NPRoomLog::Info(this, FString::Printf(TEXT("현재 방 참가자: %d명"), RoomMembers.Num()));
	for (const FNPPlayerRoomInfo& RoomMember : RoomMembers)
	{
		const FString PlayerName = RoomMember.PlayerState
			? RoomMember.PlayerState->GetPlayerName()
			: TEXT("Unknown");
		const TCHAR* PlayerRole = RoomMember.bIsHost ? TEXT("Host") : TEXT("Guest");
		NPRoomLog::Info(this, FString::Printf(TEXT("- %s: %s"), *PlayerName, PlayerRole));
	}
}

bool UNPRoomPlayerComponent::IsRoomHost() const
{
	const ANPPlayerController* PlayerController = GetNPPlayerController();
	const ANPRoomGameState* RoomGameState = PlayerController && PlayerController->GetWorld()
		? PlayerController->GetWorld()->GetGameState<ANPRoomGameState>()
		: nullptr;
	if (RoomGameState)
	{
		return RoomGameState->IsRoomHost(PlayerController->PlayerState);
	}

	return PlayerController
		&& PlayerController->GetNetMode() == NM_ListenServer
		&& PlayerController->IsLocalController();
}

bool UNPRoomPlayerComponent::CanStartGame() const
{
	const ANPPlayerController* PlayerController = GetNPPlayerController();
	const ANPRoomGameState* RoomGameState = PlayerController && PlayerController->GetWorld()
		? PlayerController->GetWorld()->GetGameState<ANPRoomGameState>()
		: nullptr;
	return IsRoomHost() && RoomGameState && RoomGameState->CanHostStartGame();
}

void UNPRoomPlayerComponent::ServerRequestStartGame_Implementation()
{
	ANPPlayerController* PlayerController = GetNPPlayerController();
	NPRoomLog::Info(
		this,
		FString::Printf(
			TEXT("게임 시작 Server RPC 도착: Player=%s"),
			PlayerController ? *GetNameSafe(PlayerController->PlayerState.Get()) : TEXT("Unknown")));

	if (ANPRoomGameMode* RoomGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ANPRoomGameMode>() : nullptr)
	{
		RoomGameMode->TryStartGame(PlayerController);
		return;
	}

	NPRoomLog::Warning(this, TEXT("게임 시작 처리 실패: ANPRoomGameMode를 찾지 못했습니다."));
}

void UNPRoomPlayerComponent::ServerRequestRestartRoom_Implementation()
{
	ANPPlayerController* PlayerController = GetNPPlayerController();
	if (ANPMainGameMode* MainGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ANPMainGameMode>() : nullptr)
	{
		MainGameMode->RequestRestartRoom(PlayerController);
		return;
	}

	NPRoomLog::Warning(this, TEXT("대기방 복귀 실패: ANPMainGameMode를 찾지 못했습니다."));
}

void UNPRoomPlayerComponent::ServerRequestExitRoom_Implementation()
{
	ANPPlayerController* PlayerController = GetNPPlayerController();
	if (ANPRoomGameMode* RoomGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ANPRoomGameMode>() : nullptr)
	{
		RoomGameMode->RequestExitRoom(PlayerController);
		return;
	}
	
	if (ANPMainGameMode* MainGameMode =	GetWorld() ? GetWorld()->GetAuthGameMode<ANPMainGameMode>() : nullptr)
	{
		ClientLeaveRoom();
		return;
	}

	NPRoomLog::Warning(this, TEXT("호스트 방 나가기 실패: ANPRoomGameMode를 찾지 못했습니다."));
}

void UNPRoomPlayerComponent::ClientBeginHostMigration_Implementation(
	const FString& MigrationId,
	const bool bBecomeHost)
{
	const ANPPlayerController* PlayerController = GetNPPlayerController();
	UGameInstance* GameInstance = PlayerController ? PlayerController->GetGameInstance() : nullptr;
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem)
	{
		NPRoomLog::Warning(this, TEXT("호스트 이전 실패: RoomSubsystem을 찾지 못했습니다."));
		return;
	}

	const FString MenuLevelPath = GetMenuLevelPath();
	if (MenuLevelPath.IsEmpty())
	{
		NPRoomLog::Warning(this, TEXT("호스트 이전 실패: RoomComponent의 MenuLevel이 지정되지 않았습니다."));
		return;
	}

	RoomSubsystem->BeginHostMigration(MigrationId, bBecomeHost, MenuLevelPath);
}

void UNPRoomPlayerComponent::ClientLeaveRoom_Implementation()
{
	const ANPPlayerController* PlayerController = GetNPPlayerController();
	UGameInstance* GameInstance = PlayerController ? PlayerController->GetGameInstance() : nullptr;
	UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!RoomSubsystem)
	{
		NPRoomLog::Warning(this, TEXT("방 나가기 실패: RoomSubsystem을 찾지 못했습니다."));
		return;
	}

	const FString MenuLevelPath = GetMenuLevelPath();
	if (MenuLevelPath.IsEmpty())
	{
		NPRoomLog::Warning(this, TEXT("방 나가기 실패: RoomComponent의 MenuLevel이 지정되지 않았습니다."));
		return;
	}

	RoomSubsystem->LeaveRoom(MenuLevelPath);
}

ANPPlayerController* UNPRoomPlayerComponent::GetNPPlayerController() const
{
	return Cast<ANPPlayerController>(GetOwner());
}

FString UNPRoomPlayerComponent::GetMenuLevelPath() const
{
	return MenuLevel.IsNull()
		? FString()
		: MenuLevel.ToSoftObjectPath().GetLongPackageName();
}
