#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPBonusRelicWidget.generated.h"

class UVerticalBox;
class UNPBonusItemWidget;

UCLASS()
class NOPHOTOS_API UNPBonusRelicWidget : public UNPUserWidget
{
	GENERATED_BODY()

public:
	UNPBonusRelicWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> BonusItemVerticalBox;

	UPROPERTY(EditAnywhere, Category = "UI|BonusRelic")
	TSubclassOf<UNPBonusItemWidget> BonusItemWidgetClass;

	void InitBonusItemList();
};