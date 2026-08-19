#include "NPRoomGameState.h"

#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "NPRoomLog.h"

void ANPRoomGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	if (HasAuthority())
	{
		RefreshCanHostStartGame();
	}
	NotifyRoomStateChanged();
}

void ANPRoomGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	if (HasAuthority())
	{
		if (HostPlayerState == PlayerState)
		{
			HostPlayerState = nullptr;
			ForceNetUpdate();
		}
		RefreshCanHostStartGame();
	}
	NotifyRoomStateChanged();
}

void ANPRoomGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPRoomGameState, HostPlayerState);
	DOREPLIFETIME(ANPRoomGameState, bCanHostStartGame);
}

TArray<FNPPlayerRoomInfo> ANPRoomGameState::GetRoomMembers() const
{
	TArray<FNPPlayerRoomInfo> RoomMembers;
	if (!IsValid(HostPlayerState.Get()))
	{
		return RoomMembers;
	}

	RoomMembers.Reserve(PlayerArray.Num());

	for (APlayerState* PlayerState : PlayerArray)
	{
		if (!IsValid(PlayerState))
		{
			continue;
		}

		FNPPlayerRoomInfo& RoomMember = RoomMembers.AddDefaulted_GetRef();
		RoomMember.PlayerState = PlayerState;
		RoomMember.bIsHost = PlayerState == HostPlayerState;
	}

	return RoomMembers;
}

bool ANPRoomGameState::IsRoomHost(const APlayerState* PlayerState) const
{
	return PlayerState && PlayerState == HostPlayerState;
}

bool ANPRoomGameState::CanHostStartGame() const
{
	return bCanHostStartGame;
}

APlayerState* ANPRoomGameState::GetHostPlayerState() const
{
	return HostPlayerState;
}

void ANPRoomGameState::SetHostPlayerState(APlayerState* PlayerState)
{
	if (!HasAuthority() || HostPlayerState == PlayerState)
	{
		return;
	}

	HostPlayerState = PlayerState;
	RefreshCanHostStartGame();
	NPRoomLog::Info(
		this,
		FString::Printf(
			TEXT("방 호스트 변경: Player=%s"),
			HostPlayerState ? *HostPlayerState->GetPlayerName() : TEXT("None")));
	NotifyRoomStateChanged();
}

void ANPRoomGameState::RefreshCanHostStartGame()
{
	if (!HasAuthority())
	{
		return;
	}

	int32 GuestCount = 0;
	for (APlayerState* PlayerState : PlayerArray)
	{
		if (!PlayerState || PlayerState == HostPlayerState)
		{
			continue;
		}

		++GuestCount;
	}

	const bool bNewCanHostStartGame = IsValid(HostPlayerState.Get()) && GuestCount > 0;
	if (bCanHostStartGame == bNewCanHostStartGame)
	{
		return;
	}

	bCanHostStartGame = bNewCanHostStartGame;
	ForceNetUpdate();
	NPRoomLog::Info(
		this,
		FString::Printf(
			TEXT("게임 시작 가능 상태 변경: GuestCount=%d, CanStart=%s"),
			GuestCount,
			bCanHostStartGame ? TEXT("true") : TEXT("false")));
}

void ANPRoomGameState::OnRep_HostPlayerState()
{
	NotifyRoomStateChanged();
}

void ANPRoomGameState::OnRep_CanHostStartGame()
{
	NPRoomLog::Info(
		this,
		FString::Printf(TEXT("게임 시작 가능 상태 복제 수신: CanStart=%s"), bCanHostStartGame ? TEXT("true") : TEXT("false")));
	NotifyRoomStateChanged();
}

void ANPRoomGameState::NotifyRoomStateChanged()
{
	OnRoomStateChanged.Broadcast();
}
