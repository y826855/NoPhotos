#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPPersonalResultWidget.generated.h"

class UHorizontalBox;
class UTextBlock;

UCLASS()
class NOPHOTOS_API UNPPersonalResultWidget : public UNPUserWidget
{
	GENERATED_BODY()

public:
	void SetupResult(int32 InRank, const FString& InPlayerName, int32 InScore);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RankText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> RelicList;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ScoreText;
};