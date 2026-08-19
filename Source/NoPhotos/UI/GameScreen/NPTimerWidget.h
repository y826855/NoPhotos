#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPTimerWidget.generated.h"

class UTextBlock;

UCLASS()
class NOPHOTOS_API UNPTimerWidget : public UNPUserWidget
{
	GENERATED_BODY()

public:
	UNPTimerWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MinuteText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SecondText;

	FTimerHandle CountdownTimerHandle;
	int32 RemainingTimeSeconds;

	void UpdateTimerUI();
};