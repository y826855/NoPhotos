#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPTimerWidget.generated.h"

class ANPMainGameState;
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
	
	FTimerHandle GameStateBindRetryTimerHandle;
	TWeakObjectPtr<ANPMainGameState> BoundMainGameState;

	UFUNCTION()
	void OnMainGameStateChanged();
	bool TryBindToMainGameState();
	void RetryBindToMainGameState();
	void UnbindFromMainGameState();
	void UpdateTimerUI(int32 RemainingTimeSeconds);
};