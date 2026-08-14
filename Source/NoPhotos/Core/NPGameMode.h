// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NPGameMode.generated.h"

class APlayerState;
class UWorld;

UCLASS()
class NOPHOTOS_API ANPGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ANPGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	bool ActivateRoom(APlayerController* HostPlayer);
	void SetPlayerReady(APlayerController* PlayerController, bool bIsReady);
	void TryStartGame(APlayerController* RequestingPlayer);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room")
	TSoftObjectPtr<UWorld> GameLevel;

private:
	bool bRoomActive = false;

	UPROPERTY()
	TObjectPtr<APlayerState> HostPlayerState;
};
