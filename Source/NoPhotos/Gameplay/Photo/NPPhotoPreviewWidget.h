#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NPPhotoPreviewWidget.generated.h"

class UImage;
class UTextureRenderTarget2D;

/** 촬영된 Render Target을 표시하는 WBP의 C++ 기반 클래스입니다. */
UCLASS(Abstract, Blueprintable)
class NOPHOTOS_API UNPPhotoPreviewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Photo")
	void ShowPhoto(UTextureRenderTarget2D* InPhoto);

	UFUNCTION(BlueprintCallable, Category="Photo")
	void HidePhoto();

	UFUNCTION(BlueprintPure, Category="Photo")
	UTextureRenderTarget2D* GetDisplayedPhoto() const { return DisplayedPhoto; }

protected:
	/** WBP에 같은 이름의 Image가 있으면 C++에서 Render Target을 바로 연결합니다. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> PhotoImage;

	/** WBP에서 플래시, 페이드 등의 연출을 시작할 수 있습니다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Photo", meta=(DisplayName="On Photo Displayed"))
	void BP_OnPhotoDisplayed(UTextureRenderTarget2D* Photo);

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> DisplayedPhoto;
};
