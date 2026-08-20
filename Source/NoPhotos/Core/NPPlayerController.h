// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NPPlayerController.generated.h"

class UInputMappingContext;
class UNPRoomPlayerComponent;
class UUserWidget;
class UNPUserWidget;

UCLASS()
class NOPHOTOS_API ANPPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANPPlayerController();

	UFUNCTION(BlueprintPure, Category = "Room")
	UNPRoomPlayerComponent* GetRoomComponent() const;

	UFUNCTION(BlueprintCallable, Category = "Room")
	bool HostRoom();

	UFUNCTION(BlueprintCallable, Category = "Room")
	bool FindRooms();

	UFUNCTION(BlueprintCallable, Category = "Room")
	bool JoinRoom(int32 RoomNumber);

	UFUNCTION(BlueprintCallable, Category = "Room")
	void RequestStartGame();

	UFUNCTION(BlueprintCallable, Category = "Room")
	void RequestRestartRoom();

	UFUNCTION(BlueprintCallable, Category = "Room")
	void ExitRoom();

	UFUNCTION(BlueprintCallable, Category = "Room")
	void ShowRoomUsers() const;

	UFUNCTION(BlueprintPure, Category = "Room")
	bool IsRoomHost() const;

	UFUNCTION(BlueprintPure, Category = "Room")
	bool CanStartGame() const;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	bool ShouldUseTouchControls() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UNPRoomPlayerComponent> RoomComponent;

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> MobileExcludedMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;
	UFUNCTION(Client, Reliable)
	void ClientLeaveRoom();
	
#pragma region UI
public: 
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMainMenuUI();
	UFUNCTION(Client, Reliable)
	void ClientShowMainMenuUI();
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowLobbyUI();
	UFUNCTION(Client, Reliable)
	void ClientShowLobbyUI();
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
	TSubclassOf<UNPUserWidget> MainMenuWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPUserWidget> LobbyWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UNPUserWidget> GameScreenWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPUserWidget> SelectPictureWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UNPUserWidget> ResultWidgetClass;
	
private:
	void ShowSingleScreen(TSubclassOf<UNPUserWidget> WidgetClass);
#pragma endregion
};
