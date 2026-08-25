#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPIngameRankObjectWidget.generated.h"

class UTextBlock;

UCLASS()
class NOPHOTOS_API UNPIngameRankObjectWidget : public UNPUserWidget
{
	GENERATED_BODY()

public:
	void SetPersonalScore(int32 InRank, const FString& InPlayerName, int32 InScore);

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PersonalScore;
};
