//작은 사진

#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPPictureWidget.generated.h"

class UButton;
class UImage;
class UTexture2D;
class UNPPictureWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FNPPictureWidgetClicked,
	UNPPictureWidget*,
	ClickedPictureWidget);

UCLASS()
class NOPHOTOS_API UNPPictureWidget : public UNPUserWidget
{
	GENERATED_BODY()

public:
	void SetPicture(UTexture2D* InTexture);
	void SetSelected(bool bInSelected);

	UPROPERTY(BlueprintAssignable, Category = "Picture")
	FNPPictureWidgetClicked OnPictureClicked;

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleButtonClicked();

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool IsSelected = false;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Picture;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SelectMark;
};