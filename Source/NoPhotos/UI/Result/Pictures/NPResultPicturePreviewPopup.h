#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPResultPicturePreviewPopup.generated.h"

class UButton;
class UImage;
class UTexture2D;

/** WBP_ResultPicturePreviewPopup의 부모 클래스입니다. */
UCLASS()
class NOPHOTOS_API UNPResultPicturePreviewPopup : public UNPUserWidget
{
	GENERATED_BODY()

public:
	void OpenWithTexture(UTexture2D* InTexture);

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PreviewImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;
};
