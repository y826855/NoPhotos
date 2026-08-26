// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/NPPlayerController.h"
#include "NPRoomPlayerController.generated.h"

class UNPUserWidget;
class UInputMappingContext;
class UInputAction;

/** 타이틀과 로비 UI를 담당하는 PlayerController입니다. */
UCLASS()
class NOPHOTOS_API ANPRoomPlayerController : public ANPPlayerController
{
	GENERATED_BODY()

public:
	ANPRoomPlayerController();

	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* InputMappingContext;
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ChangeInputAction;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMainMenuUI();

	UFUNCTION(Client, Reliable)
	void ClientShowMainMenuUI();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowLobbyUI();

	UFUNCTION(Client, Reliable)
	void ClientShowLobbyUI();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPUserWidget> LobbyWidgetClass;

	bool bIsMouseInput = false;
	void ChangeInputMode();
	virtual void SetupInputComponent() override;

private:
	void SetLobbyInputMappingEnabled(bool bEnabled);
	void ApplyLobbyInputMode();
	void ShowSingleScreen(TSubclassOf<UNPUserWidget> WidgetClass);
};
