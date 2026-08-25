#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "UI/NPUserWidget.h"
#include "NPIngameRankListWidget.generated.h"

class UVerticalBox;
class ANPMainGameState;
class UNPIngameRankObjectWidget;

UCLASS()
class NOPHOTOS_API UNPIngameRankListWidget : public UNPUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Score")
	void SetRankList();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandlePlayerRankingsChanged();

	void TryInitializeRankList();
	bool TryBindToMainGameState();

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UVerticalBox> RankList;
	UPROPERTY(Transient) //데이터 안남길때 사용
	TObjectPtr<ANPMainGameState> MainGameState;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPIngameRankObjectWidget> RankObjectWidgetClass;

	FTimerHandle RankListInitializationTimer;
};
