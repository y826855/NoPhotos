// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NoPhotosPlayerController.h"
#include "NPMainPlayerController.generated.h"

class UNPUserWidget;

/** 게임 플레이, 사진 선택, 정산 화면을 담당하는 PlayerController입니다. */
UCLASS()
class NOPHOTOS_API ANPMainPlayerController : public ANoPhotosPlayerController
{
	GENERATED_BODY()

public:
	ANPMainPlayerController();

	UFUNCTION(BlueprintPure, Category = "Room")
	bool IsListenServerHost() const;

	UFUNCTION(BlueprintCallable, Category = "Room")
	void RequestRestartRoom();
	UFUNCTION(BlueprintCallable, Category = "Room")
    void ExitToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowGameScreenUI();
	UFUNCTION(Client, Reliable)
	void ClientShowGameScreenUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowSelectPictureUI();
	UFUNCTION(Client, Reliable)
	void ClientShowSelectPictureUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowResultUI();
	UFUNCTION(Client, Reliable)
	void ClientShowResultUI();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPUserWidget> GameScreenWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPUserWidget> SelectPictureWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPUserWidget> ResultWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Room")
    TSoftObjectPtr<UWorld> MainMenuLevel;

private:
	UFUNCTION(Server, Reliable)
	void ServerRequestRestartRoom();

	void ShowSingleScreen(TSubclassOf<UNPUserWidget> WidgetClass);
};
