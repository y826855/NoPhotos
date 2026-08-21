#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPPersonalResultWidget.generated.h"

class APlayerState;
class UButton;
class UHorizontalBox;
class UTextBlock;
class UUserWidget;
class UNPResultPicturePopup;

UCLASS()
class NOPHOTOS_API UNPPersonalResultWidget : public UNPUserWidget
{
	GENERATED_BODY()

public:
	void SetupResult(int32 InRank, const FString& InPlayerName,	int32 InScore,	APlayerState* InPlayerState);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleShowPictureButtonClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RankText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> RelicList;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ScoreText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ShowPictureButton;

	// UNPResultPicturePopup을 부모로 한 WBP_ResultPicturePopup을 여기에서 지정합니다.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPResultPicturePopup> PicturePopupWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<APlayerState> ResultPlayerState;
};
