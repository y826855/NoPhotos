#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPJoinPlayer.generated.h"

class UTextBlock;

UCLASS()
class NOPHOTOS_API UNPJoinPlayer : public UNPUserWidget
{
	GENERATED_BODY()
	
public:
	void SetupResult(int32 InNumber, const FString& InPlayerName);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> JoinNumberText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;
};
