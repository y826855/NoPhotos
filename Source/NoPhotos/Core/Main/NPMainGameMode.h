#pragma once

#include "CoreMinimal.h"
#include "NoPhotosGameMode.h"
#include "NPMainGameMode.generated.h"

class UWorld;

UCLASS()
class NOPHOTOS_API ANPMainGameMode : public ANoPhotosGameMode
{
	GENERATED_BODY()

public:
	ANPMainGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleSeamlessTravelPlayer(AController*& Controller) override;

	void RequestRestartRoom(APlayerController* RequestingPlayer);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Game", meta = (ClampMin = "1"))
	int32 GameDurationSeconds = 180;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Main Game")
	TSoftObjectPtr<UWorld> RoomLevel;

private:
	void StartMainGame();
	void UpdateMainGameTimer();
	void RefreshPlayerRankings();
	void TryAwardInitialScore();
	void HandleWaitingRoomRestored(bool bWasSuccessful);

	FTimerHandle MainGameTimer;
	bool bInitialScoreAwarded = false;
	bool bReturningToRoom = false;
};
