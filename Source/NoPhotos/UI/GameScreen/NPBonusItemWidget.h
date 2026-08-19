#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPBonusItemWidget.generated.h"

class UTextBlock;

UCLASS()
class NOPHOTOS_API UNPBonusItemWidget : public UNPUserWidget
{
	GENERATED_BODY()

public:
	UNPBonusItemWidget(const FObjectInitializer& ObjectInitializer);

	// 추후 MVVM 바인딩 시 아이템 정보(이름)를 세팅할 함수
	void SetItemName(const FString& InItemName);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;
};