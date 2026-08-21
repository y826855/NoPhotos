#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPJoinPlayerList.generated.h"

class UVerticalBox;
class UNPJoinPlayer;
class ANPRoomGameState;

UCLASS()
class NOPHOTOS_API UNPJoinPlayerList : public UNPUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> PlayerList;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPJoinPlayer> JoinPlayerWidgetClass;
	
	FTimerHandle PlayerNameRefreshTimer;
	TWeakObjectPtr<ANPRoomGameState> BoundRoomGameState;
	
	UFUNCTION()
	void OnRoomStateChanged();
	void RefreshPlayerList();
};
