// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NPPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class NOPHOTOS_API ANPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANPPlayerController();

	UFUNCTION(BlueprintCallable, Category = "Room")
	bool HostRoom();

	UFUNCTION(BlueprintCallable, Category = "Room")
	bool JoinLocalRoom();

	UFUNCTION(BlueprintCallable, Category = "Room")
	void SetReady(bool bIsReady);

	UFUNCTION(BlueprintCallable, Category = "Room")
	void RequestStartGame();

	UFUNCTION(BlueprintPure, Category = "Room")
	bool IsRoomHost() const;

	UFUNCTION(BlueprintPure, Category = "Room")
	bool CanStartGame() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerSetReady(bool bIsReady);

	UFUNCTION(Server, Reliable)
	void ServerRequestStartGame();
};
