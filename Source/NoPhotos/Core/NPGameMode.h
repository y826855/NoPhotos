#pragma once

#include "CoreMinimal.h"
#include "Room/NPRoomGameMode.h"
#include "NPGameMode.generated.h"

class APlayerState;
class ANPPlayerController;
class UWorld;

UCLASS()
class NOPHOTOS_API ANPGameMode : public ANPRoomGameMode
{
	GENERATED_BODY()

public:
	ANPGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	bool ActivateRoom(APlayerController* HostPlayer);
	void SetPlayerReady(APlayerController* PlayerController, bool bIsReady);
	void TryStartGame(APlayerController* RequestingPlayer);
	void RequestExitRoom(ANPPlayerController* RequestingPlayer);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room")
	TSoftObjectPtr<UWorld> GameLevel;

private:
	void FinishHostMigrationExit();

	bool bRoomActive = false;

	UPROPERTY()
	TObjectPtr<APlayerState> HostPlayerState;

	UPROPERTY()
	TObjectPtr<ANPPlayerController> PendingExitingHost;

	FTimerHandle HostMigrationExitTimer;

};
