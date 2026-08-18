// Fill out your copyright notice in the Description page of Project Settings.


#include "NPGameState.h"

#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Room/NPRoomLog.h"

void ANPGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPGameState, RoomMembers);
	DOREPLIFETIME(ANPGameState, bCanHostStartGame);
}

TArray<FNPPlayerRoomInfo> ANPGameState::GetRoomMembers() const
{
	return RoomMembers;
}

bool ANPGameState::IsRoomHost(const APlayerState* PlayerState) const
{
	const FNPPlayerRoomInfo* RoomMember = RoomMembers.FindByPredicate(
		[PlayerState](const FNPPlayerRoomInfo& Member)
		{
			return Member.PlayerState == PlayerState;
		});

	return RoomMember && RoomMember->bIsHost;
}

bool ANPGameState::IsRoomMemberReady(const APlayerState* PlayerState) const
{
	const FNPPlayerRoomInfo* RoomMember = RoomMembers.FindByPredicate(
		[PlayerState](const FNPPlayerRoomInfo& Member)
		{
			return Member.PlayerState == PlayerState;
		});

	return RoomMember && RoomMember->bIsReady;
}

bool ANPGameState::CanHostStartGame() const
{
	return bCanHostStartGame;
}

void ANPGameState::AddRoomMember(APlayerState* PlayerState, const bool bIsHost)
{
	if (!HasAuthority() || !IsValid(PlayerState))
	{
		NPRoomLog::Warning(this, TEXT("방 멤버 추가 실패: 서버 권한이 없거나 PlayerState가 유효하지 않습니다."));
		return;
	}

	FNPPlayerRoomInfo* ExistingMember = RoomMembers.FindByPredicate(
		[PlayerState](const FNPPlayerRoomInfo& Member)
		{
			return Member.PlayerState == PlayerState;
		});

	if (ExistingMember)
	{
		ExistingMember->bIsHost = bIsHost;
	}
	else
	{
		FNPPlayerRoomInfo& NewMember = RoomMembers.AddDefaulted_GetRef();
		NewMember.PlayerState = PlayerState;
		NewMember.bIsHost = bIsHost;
	}

	RefreshCanHostStartGame();
	NPRoomLog::Info(
		this,
		FString::Printf(
			TEXT("방 멤버 상태 추가: Player=%s, Role=%s, MemberCount=%d"),
			*PlayerState->GetPlayerName(),
			bIsHost ? TEXT("Host") : TEXT("Guest"),
			RoomMembers.Num()));
	NotifyRoomStateChanged();
}

void ANPGameState::RemoveRoomMember(const APlayerState* PlayerState)
{
	if (!HasAuthority() || !PlayerState)
	{
		NPRoomLog::Warning(this, TEXT("방 멤버 제거 실패: 서버 권한이 없거나 PlayerState가 없습니다."));
		return;
	}

	const int32 RemovedCount = RoomMembers.RemoveAll(
		[PlayerState](const FNPPlayerRoomInfo& Member)
		{
			return Member.PlayerState == PlayerState;
		});

	if (RemovedCount > 0)
	{
		RefreshCanHostStartGame();
		NPRoomLog::Info(
			this,
			FString::Printf(TEXT("방 멤버 제거: Player=%s, MemberCount=%d"), *PlayerState->GetPlayerName(), RoomMembers.Num()));
		NotifyRoomStateChanged();
		return;
	}

	NPRoomLog::Warning(this, FString::Printf(TEXT("방 멤버 제거 실패: 목록에 없는 Player=%s"), *PlayerState->GetPlayerName()));
}

void ANPGameState::SetRoomMemberReady(const APlayerState* PlayerState, const bool bIsReady)
{
	if (!HasAuthority() || !PlayerState)
	{
		NPRoomLog::Warning(this, TEXT("준비 상태 변경 실패: 서버 권한이 없거나 PlayerState가 없습니다."));
		return;
	}

	FNPPlayerRoomInfo* RoomMember = RoomMembers.FindByPredicate(
		[PlayerState](const FNPPlayerRoomInfo& Member)
		{
			return Member.PlayerState == PlayerState;
		});

	if (!RoomMember)
	{
		NPRoomLog::Warning(this, FString::Printf(TEXT("준비 상태 변경 실패: 목록에 없는 Player=%s"), *PlayerState->GetPlayerName()));
		return;
	}

	if (RoomMember->bIsHost)
	{
		NPRoomLog::Warning(this, TEXT("준비 상태 변경 무시: 호스트는 준비 대상이 아닙니다."));
		return;
	}

	if (RoomMember->bIsReady == bIsReady)
	{
		NPRoomLog::Info(
			this,
			FString::Printf(TEXT("준비 상태 변경 없음: Player=%s, Ready=%s"), *PlayerState->GetPlayerName(), bIsReady ? TEXT("true") : TEXT("false")));
		return;
	}

	RoomMember->bIsReady = bIsReady;
	RefreshCanHostStartGame();
	NPRoomLog::Info(
		this,
		FString::Printf(TEXT("준비 상태 적용: Player=%s, Ready=%s"), *PlayerState->GetPlayerName(), bIsReady ? TEXT("true") : TEXT("false")));
	NotifyRoomStateChanged();
}

void ANPGameState::OnRep_RoomMembers()
{
	NPRoomLog::Info(this, FString::Printf(TEXT("참가자 목록 복제 수신: MemberCount=%d"), RoomMembers.Num()));
	OnRoomStateChanged.Broadcast();
}

void ANPGameState::OnRep_CanHostStartGame()
{
	NPRoomLog::Info(
		this,
		FString::Printf(TEXT("게임 시작 가능 상태 복제 수신: CanStart=%s"), bCanHostStartGame ? TEXT("true") : TEXT("false")));
	OnRoomStateChanged.Broadcast();
}

void ANPGameState::RefreshCanHostStartGame()
{
	const bool bPreviousCanHostStartGame = bCanHostStartGame;
	int32 GuestCount = 0;
	bool bEveryGuestIsReady = true;

	for (const FNPPlayerRoomInfo& Member : RoomMembers)
	{
		if (!Member.bIsHost)
		{
			++GuestCount;
			bEveryGuestIsReady &= Member.bIsReady;
		}
	}

	bCanHostStartGame = GuestCount > 0 && bEveryGuestIsReady;

	if (bPreviousCanHostStartGame != bCanHostStartGame)
	{
		NPRoomLog::Info(
			this,
			FString::Printf(
				TEXT("게임 시작 가능 상태 변경: GuestCount=%d, CanStart=%s"),
				GuestCount,
				bCanHostStartGame ? TEXT("true") : TEXT("false")));
	}
}

void ANPGameState::NotifyRoomStateChanged()
{
	ForceNetUpdate();
	OnRoomStateChanged.Broadcast();
}

