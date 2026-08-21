#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPRandomRoomManager.generated.h"

class UArrowComponent;
class ULevelStreamingDynamic;
class USceneComponent;
class UWorld;

UCLASS(Blueprintable)
class NOPHOTOS_API ANPRandomRoomManager : public AActor
{
	GENERATED_BODY()

public:
	ANPRandomRoomManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room Layout")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room Layout|Slots")
	TObjectPtr<UArrowComponent> TopRightSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room Layout|Slots")
	TObjectPtr<UArrowComponent> TopLeftSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room Layout|Slots")
	TObjectPtr<UArrowComponent> BottomRightSlot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room Layout|Slots")
	TObjectPtr<UArrowComponent> BottomLeftSlot;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room Layout")
	TArray<TSoftObjectPtr<UWorld>> Rooms;

private:
	UFUNCTION()
	void OnRep_LayoutSeed();

	void LoadRandomRoomLayout();

	UPROPERTY(ReplicatedUsing = OnRep_LayoutSeed)
	int32 LayoutSeed = 0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULevelStreamingDynamic>> LoadedRooms;

	bool bLayoutLoaded = false;
};
