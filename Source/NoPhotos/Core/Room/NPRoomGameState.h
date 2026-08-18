#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "NPRoomGameState.generated.h"

class APlayerState;

USTRUCT(BlueprintType)
struct NOPHOTOS_API FNPPlayerRoomInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	bool bIsHost = false;

};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNPOnRoomStateChanged);

UCLASS()
class NOPHOTOS_API ANPRoomGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Room")
	TArray<FNPPlayerRoomInfo> GetRoomMembers() const;

	UFUNCTION(BlueprintPure, Category = "Room")
	bool IsRoomHost(const APlayerState* PlayerState) const;

	UFUNCTION(BlueprintPure, Category = "Room")
	bool CanHostStartGame() const;

	UFUNCTION(BlueprintPure, Category = "Room")
	APlayerState* GetHostPlayerState() const;

	UPROPERTY(BlueprintAssignable, Category = "Room")
	FNPOnRoomStateChanged OnRoomStateChanged;

	void SetHostPlayerState(APlayerState* PlayerState);
	void RefreshCanHostStartGame();

private:
	UFUNCTION()
	void OnRep_HostPlayerState();

	UFUNCTION()
	void OnRep_CanHostStartGame();

	void NotifyRoomStateChanged();

	UPROPERTY(ReplicatedUsing = OnRep_HostPlayerState)
	TObjectPtr<APlayerState> HostPlayerState;

	UPROPERTY(ReplicatedUsing = OnRep_CanHostStartGame)
	bool bCanHostStartGame = false;
};
