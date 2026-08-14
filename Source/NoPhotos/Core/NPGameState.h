// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "NPGameState.generated.h"

class APlayerState;

USTRUCT(BlueprintType)
struct NOPHOTOS_API FNPPlayerRoomInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	bool bIsHost = false;

	UPROPERTY(BlueprintReadOnly, Category = "Room")
	bool bIsReady = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNPOnRoomStateChanged);

UCLASS()
class NOPHOTOS_API ANPGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Room")
	TArray<FNPPlayerRoomInfo> GetRoomMembers() const;

	UFUNCTION(BlueprintPure, Category = "Room")
	bool IsRoomHost(const APlayerState* PlayerState) const;

	UFUNCTION(BlueprintPure, Category = "Room")
	bool IsRoomMemberReady(const APlayerState* PlayerState) const;

	UFUNCTION(BlueprintPure, Category = "Room")
	bool CanHostStartGame() const;

	UPROPERTY(BlueprintAssignable, Category = "Room")
	FNPOnRoomStateChanged OnRoomStateChanged;

	void AddRoomMember(APlayerState* PlayerState, bool bIsHost);
	void RemoveRoomMember(const APlayerState* PlayerState);
	void SetRoomMemberReady(const APlayerState* PlayerState, bool bIsReady);

private:
	UFUNCTION()
	void OnRep_RoomMembers();

	UFUNCTION()
	void OnRep_CanHostStartGame();

	void RefreshCanHostStartGame();
	void NotifyRoomStateChanged();

	UPROPERTY(ReplicatedUsing = OnRep_RoomMembers)
	TArray<FNPPlayerRoomInfo> RoomMembers;

	UPROPERTY(ReplicatedUsing = OnRep_CanHostStartGame)
	bool bCanHostStartGame = false;
};
