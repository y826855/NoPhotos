#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "NPMainGameState.generated.h"

class APlayerController;
class APlayerState;
class ANPPlayerState;

USTRUCT(BlueprintType)
struct NOPHOTOS_API FNPPlayerRanking
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	TObjectPtr<ANPPlayerState> PlayerState = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Score")
	int32 Score = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNPOnPlayerRankingsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNPOnMainGameStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNPOnPictureSelectionStateChanged);

UCLASS()
class NOPHOTOS_API ANPMainGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Main Game")
	TArray<FNPPlayerRanking> GetPlayerRankings() const;

	UFUNCTION(BlueprintPure, Category = "Main Game")
	int32 GetRemainingGameTime() const;

	UFUNCTION(BlueprintPure, Category = "Main Game")
	bool IsMainGameActive() const;

	UFUNCTION(BlueprintPure, Category = "Main Game")
	bool IsMainGameEnded() const;

	UFUNCTION(BlueprintPure, Category = "Picture Selection")
	bool IsPlayerPictureSelectionComplete(const APlayerState* PlayerState) const;

	//모든 플레이어가 사진 선택을 완료했으면 정산 화면으로 전환
	void ConfirmPictureSelection(APlayerController* PlayerController);

	UPROPERTY(BlueprintAssignable, Category = "Main Game")
	FNPOnPlayerRankingsChanged OnPlayerRankingsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Main Game")
	FNPOnMainGameStateChanged OnMainGameStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Picture Selection")
	FNPOnPictureSelectionStateChanged OnPictureSelectionStateChanged;

	void RefreshPlayerRankings();
	void StartMainGame(int32 DurationSeconds);
	void SetRemainingGameTime(int32 RemainingSeconds);
	void FinishMainGame();

private:
	UFUNCTION()
	void OnRep_PlayerRankings();

	UFUNCTION()
	void OnRep_MainGameState();

	UFUNCTION()
	void OnRep_PictureSelectionCompletedPlayers();

	bool AreAllConnectedPlayersPictureSelectionComplete() const;

	void LogLocalGameStatus();
	void TryLogFinalRankings();
	void ShowPlayerRankingsDebugMessage() const;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerRankings)
	TArray<FNPPlayerRanking> PlayerRankings;

	UPROPERTY(ReplicatedUsing = OnRep_MainGameState)
	int32 RemainingGameTime = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MainGameState)
	bool bMainGameActive = false;

	UPROPERTY(ReplicatedUsing = OnRep_MainGameState)
	bool bMainGameEnded = false;

	//사진 선택 완료를 누른 플레이어 목록
	UPROPERTY(ReplicatedUsing = OnRep_PictureSelectionCompletedPlayers)
	TArray<TObjectPtr<APlayerState>> PictureSelectionCompletedPlayers;

	int32 LastLoggedRemainingTime = INDEX_NONE;
	bool bFinalRankingsLogged = false;
};