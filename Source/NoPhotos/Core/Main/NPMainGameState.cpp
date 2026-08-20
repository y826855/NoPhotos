#include "NPMainGameState.h"

#include "Core/NPPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "NPMainGameLog.h"
#include "Core/NPPlayerController.h"

void ANPMainGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPMainGameState, PlayerRankings);
	DOREPLIFETIME(ANPMainGameState, RemainingGameTime);
	DOREPLIFETIME(ANPMainGameState, bMainGameActive);
	DOREPLIFETIME(ANPMainGameState, bMainGameEnded);
}

TArray<FNPPlayerRanking> ANPMainGameState::GetPlayerRankings() const
{
	return PlayerRankings;
}

int32 ANPMainGameState::GetRemainingGameTime() const
{
	return RemainingGameTime;
}

bool ANPMainGameState::IsMainGameActive() const
{
	return bMainGameActive;
}

bool ANPMainGameState::IsMainGameEnded() const
{
	return bMainGameEnded;
}

void ANPMainGameState::RefreshPlayerRankings()
{
	if (!HasAuthority())
	{
		return;
	}

	PlayerRankings.Reset();
	for (APlayerState* PlayerState : PlayerArray)
	{
		ANPPlayerState* NPPlayerState = Cast<ANPPlayerState>(PlayerState);
		if (!NPPlayerState)
		{
			continue;
		}

		FNPPlayerRanking& Ranking = PlayerRankings.AddDefaulted_GetRef();
		Ranking.PlayerState = NPPlayerState;
		Ranking.Score = NPPlayerState->GetPlayerScore();
	}

	PlayerRankings.Sort(
		[](const FNPPlayerRanking& Left, const FNPPlayerRanking& Right)
		{
			if (Left.Score != Right.Score)
			{
				return Left.Score > Right.Score;
			}

			const int32 LeftPlayerId = Left.PlayerState ? Left.PlayerState->GetPlayerId() : INDEX_NONE;
			const int32 RightPlayerId = Right.PlayerState ? Right.PlayerState->GetPlayerId() : INDEX_NONE;
			return LeftPlayerId < RightPlayerId;
		});

	ForceNetUpdate();
	OnPlayerRankingsChanged.Broadcast();
}

void ANPMainGameState::StartMainGame(const int32 DurationSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	RemainingGameTime = FMath::Max(1, DurationSeconds);
	bMainGameActive = true;
	bMainGameEnded = false;
	LastLoggedRemainingTime = INDEX_NONE;
	bFinalRankingsLogged = false;
	RefreshPlayerRankings();
	ForceNetUpdate();
	OnMainGameStateChanged.Broadcast();
	LogLocalGameStatus();
}

void ANPMainGameState::SetRemainingGameTime(const int32 RemainingSeconds)
{
	if (!HasAuthority() || !bMainGameActive)
	{
		return;
	}

	RemainingGameTime = FMath::Max(0, RemainingSeconds);
	ForceNetUpdate();
	OnMainGameStateChanged.Broadcast();
	LogLocalGameStatus();
}

void ANPMainGameState::FinishMainGame()
{
	if (!HasAuthority() || bMainGameEnded)
	{
		return;
	}

	RemainingGameTime = 0;
	bMainGameActive = false;
	bMainGameEnded = true;
	RefreshPlayerRankings();
	ForceNetUpdate();
	OnMainGameStateChanged.Broadcast();
	TryLogFinalRankings();
	
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator;	++Iterator)
	{
		ANPPlayerController* NPPlayerController = Cast<ANPPlayerController>(Iterator->Get());

		if (IsValid(NPPlayerController))
		{
			NPPlayerController->ClientShowSelectPictureUI();
		}
	}
}

void ANPMainGameState::OnRep_PlayerRankings()
{
	OnPlayerRankingsChanged.Broadcast();
	TryLogFinalRankings();
}

void ANPMainGameState::OnRep_MainGameState()
{
	OnMainGameStateChanged.Broadcast();
	if (bMainGameActive)
	{
		LogLocalGameStatus();
		return;
	}

	TryLogFinalRankings();
}

void ANPMainGameState::LogLocalGameStatus()
{
	if (!bMainGameActive || LastLoggedRemainingTime == RemainingGameTime)
	{
		return;
	}

	APlayerController* LocalPlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const ANPPlayerState* LocalPlayerState = LocalPlayerController
		? LocalPlayerController->GetPlayerState<ANPPlayerState>()
		: nullptr;
	if (!LocalPlayerState || !LocalPlayerController->IsLocalController())
	{
		return;
	}

	LastLoggedRemainingTime = RemainingGameTime;
	NPMainGameLog::Info(
		this,
		FString::Printf(
			TEXT("남은 시간=%d초, 내 점수=%d점"),
			RemainingGameTime,
			LocalPlayerState->GetPlayerScore()));
}

void ANPMainGameState::TryLogFinalRankings()
{
	if (!bMainGameEnded || bFinalRankingsLogged || PlayerRankings.IsEmpty())
	{
		return;
	}

	APlayerController* LocalPlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!LocalPlayerController || !LocalPlayerController->IsLocalController())
	{
		return;
	}

	bFinalRankingsLogged = true;
	NPMainGameLog::Info(this, TEXT("게임 종료 - 최종 순위"));
	for (int32 RankingIndex = 0; RankingIndex < PlayerRankings.Num(); ++RankingIndex)
	{
		const FNPPlayerRanking& Ranking = PlayerRankings[RankingIndex];
		const FString PlayerName = Ranking.PlayerState
			? Ranking.PlayerState->GetPlayerName()
			: TEXT("Unknown");
		NPMainGameLog::Info(
			this,
			FString::Printf(
				TEXT("%d위 - %s: %d점"),
				RankingIndex + 1,
				*PlayerName,
				Ranking.Score));
	}
}
