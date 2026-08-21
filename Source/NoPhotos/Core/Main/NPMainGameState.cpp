#include "NPMainGameState.h"

#include "Core/NPPlayerState.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "NPMainGameLog.h"

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
	ShowPlayerRankingsDebugMessage();
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
}

void ANPMainGameState::OnRep_PlayerRankings()
{
	OnPlayerRankingsChanged.Broadcast();
	ShowPlayerRankingsDebugMessage();
	TryLogFinalRankings();
}

void ANPMainGameState::ShowPlayerRankingsDebugMessage() const
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (!GEngine)
	{
		return;
	}

	FString ScoreText = TEXT("=== 현재 점수 ===\n");
	if (PlayerRankings.IsEmpty())
	{
		ScoreText += TEXT("플레이어 없음");
	}
	else
	{
		for (int32 RankingIndex = 0; RankingIndex < PlayerRankings.Num(); ++RankingIndex)
		{
			const FNPPlayerRanking& Ranking = PlayerRankings[RankingIndex];
			ScoreText += FString::Printf(
				TEXT("%d. %s : %d점%s"),
				RankingIndex + 1,
				Ranking.PlayerState ? *Ranking.PlayerState->GetPlayerName() : TEXT("Unknown"),
				Ranking.Score,
				RankingIndex + 1 < PlayerRankings.Num() ? TEXT("\n") : TEXT(""));
		}
	}

	constexpr uint64 PlayerRankingsDebugMessageKey = 1001;
	GEngine->AddOnScreenDebugMessage(
		PlayerRankingsDebugMessageKey,
		10.0f,
		FColor::Cyan,
		ScoreText);
#endif
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
