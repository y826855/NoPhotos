#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPJoinPlayer.generated.h"

class UTextBlock;
class UWidgetAnimation;
class UNPJoinPlayer;

DECLARE_MULTICAST_DELEGATE_OneParam(FNPOnJoinPlayerLeaveAnimationFinished, UNPJoinPlayer*);

UCLASS()
class NOPHOTOS_API UNPJoinPlayer : public UNPUserWidget
{
	GENERATED_BODY()
	
public:
	void SetupResult(const FString& InPlayerName);
	void PlayJoinAnimation(float Delay);
	void PlayLeaveAnimation();
	bool IsLeaving() const { return bIsLeaving; }

	FNPOnJoinPlayerLeaveAnimationFinished OnLeaveAnimationFinished;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void StartJoinAnimation();

	UFUNCTION()
	void HandleLeaveAnimationFinished();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> JoinPoster;
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> LeavePoster;

	FTimerHandle JoinAnimationTimer;
	bool bIsLeaving = false;
};
