#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPRoomPlayerComponent.generated.h"

class ANPPlayerController;
class UWorld;

UCLASS(ClassGroup = (Room), meta = (BlueprintSpawnableComponent))
class NOPHOTOS_API UNPRoomPlayerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPRoomPlayerComponent();

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

	UFUNCTION(Client, Reliable)
	void ClientBeginHostMigration(const FString& MigrationId, bool bBecomeHost);

	UFUNCTION(Client, Reliable)
	void ClientLeaveRoom();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room")
	TSoftObjectPtr<UWorld> MenuLevel;

	UFUNCTION(Server, Reliable)
	void ServerRequestStartGame();

	UFUNCTION(Server, Reliable)
	void ServerRequestRestartRoom();

	UFUNCTION(Server, Reliable)
	void ServerRequestExitRoom();

private:
	ANPPlayerController* GetNPPlayerController() const;
	FString GetMenuLevelPath() const;
};
