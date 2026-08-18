#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NPPlayerController.generated.h"

class UInputMappingContext;
class UNPRoomPlayerComponent;
class UUserWidget;

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
};
