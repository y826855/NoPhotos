#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NPRoomGameMode.generated.h"

class ANPPlayerController;
class UWorld;

UCLASS()
class NOPHOTOS_API ANPRoomGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ANPRoomGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleSeamlessTravelPlayer(AController*& Controller) override;

	bool ActivateRoom(APlayerController* HostPlayer);
	void TryStartGame(APlayerController* RequestingPlayer);
	void RequestExitRoom(ANPPlayerController* RequestingPlayer);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room")
	TSoftObjectPtr<UWorld> GameLevel;

private:
	void FinishHostMigrationExit();
	bool TryActivateExistingWaitingRoom();
	void RestoreReturningPlayer(AController* Controller);

	bool bRoomActive = false;

	UPROPERTY()
	TObjectPtr<ANPPlayerController> PendingExitingHost;

	FTimerHandle HostMigrationExitTimer;
};
