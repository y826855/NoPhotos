#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPResultListWidget.generated.h"

class UVerticalBox;
class UNPPersonalResultWidget;

UCLASS()
class NOPHOTOS_API UNPResultListWidget : public UNPUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	void RefreshResultList();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> RankList;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPPersonalResultWidget> PersonalResultWidgetClass;
};