#include "UI/MainMenu/Lobby/NPJoinPlayerList.h"

#include "NPJoinPlayer.h"
#include "Components/VerticalBox.h"
#include "Core/NPPlayerState.h"
#include "Core/Room/NPRoomGameState.h"
#include "TimerManager.h"

void UNPJoinPlayerList::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshPlayerList();
}

void UNPJoinPlayerList::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlayerNameRefreshTimer);
	}

	if (ANPRoomGameState* RoomGameState = BoundRoomGameState.Get())
	{
		RoomGameState->OnRoomStateChanged.RemoveAll(this);
	}
	BoundRoomGameState.Reset();

	Super::NativeDestruct();
}

void UNPJoinPlayerList::OnRoomStateChanged()
{
	RefreshPlayerList();
}

void UNPJoinPlayerList::RefreshPlayerList()
{
    if (!IsValid(PlayerList) || !IsValid(JoinPlayerWidgetClass))
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return;
    }

    ANPRoomGameState* RoomGameState =
        World->GetGameState<ANPRoomGameState>();

    // 게스트는 UI가 먼저 열리고 GameState가 나중에 복제될 수 있다.
    if (!IsValid(RoomGameState))
    {
        World->GetTimerManager().SetTimer(
            PlayerNameRefreshTimer,
            this,
            &UNPJoinPlayerList::RefreshPlayerList,
            0.1f,
            false);

        return;
    }

	if (BoundRoomGameState.Get() != RoomGameState)
	{
		if (ANPRoomGameState* PreviousGameState = BoundRoomGameState.Get())
		{
			PreviousGameState->OnRoomStateChanged.RemoveAll(this);
		}

		BoundRoomGameState = RoomGameState;
		RoomGameState->OnRoomStateChanged.AddUniqueDynamic(
			this,
			&UNPJoinPlayerList::OnRoomStateChanged);
	}

    PlayerList->ClearChildren();

    const TArray<FNPPlayerRoomInfo> PlayerMemberList =
        RoomGameState->GetRoomMembers();

    // HostPlayerState가 아직 복제되지 않은 경우 GetRoomMembers는 빈 배열을 반환한다.
    bool bNeedRefreshAgain = PlayerMemberList.IsEmpty();

    for (int32 PlayerIndex = 0; PlayerIndex < PlayerMemberList.Num(); ++PlayerIndex)
    {
        const FNPPlayerRoomInfo& RoomMember =
            PlayerMemberList[PlayerIndex];

        FString PlayerName = TEXT("접속 중...");

        if (IsValid(RoomMember.PlayerState))
        {
            PlayerName = RoomMember.PlayerState->GetPlayerName();

            // 행은 생겼지만 PlayerName 복제가 아직 끝나지 않은 경우
            if (PlayerName.IsEmpty())
            {
                PlayerName = TEXT("접속 중...");
                bNeedRefreshAgain = true;
            }
        }
        else
        {
            bNeedRefreshAgain = true;
        }

        UNPJoinPlayer* JoinPlayerWidget =
            CreateWidget<UNPJoinPlayer>(
                GetOwningPlayer(),
                JoinPlayerWidgetClass);

        if (!IsValid(JoinPlayerWidget))
        {
            continue;
        }

        JoinPlayerWidget->SetupResult(
            PlayerIndex + 1,
            PlayerName);

        PlayerList->AddChild(JoinPlayerWidget);
    }

    // 아직 GameState / HostPlayerState / PlayerName 복제가 덜 끝났다면 다시 확인한다.
    if (bNeedRefreshAgain)
    {
        World->GetTimerManager().SetTimer(
            PlayerNameRefreshTimer,
            this,
            &UNPJoinPlayerList::RefreshPlayerList,
            0.1f,
            false);

        return;
    }

    World->GetTimerManager().ClearTimer(PlayerNameRefreshTimer);
}
