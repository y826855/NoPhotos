#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NPPhotoPreviewWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture;
class UTexture2D;
class ANoPhotosGameState;
class UNPPhotoTransferComponent;

/** 새 사진 알림을 표시하고 요청한 네트워크 사진을 2초 동안 보여주는 WBP 기반 클래스입니다. */
UCLASS(Abstract, Blueprintable)
class NOPHOTOS_API UNPPhotoPreviewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Photo")
	void ShowPhoto(UTexture* InPhoto);

	UFUNCTION(BlueprintCallable, Category="Photo")
	void HidePhoto();

	UFUNCTION(BlueprintPure, Category="Photo")
	UTexture* GetDisplayedPhoto() const { return DisplayedPhoto; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> PhotoImage;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> PhotoReceiveButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PhotoTransferText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Photo", meta=(ClampMin="0.0"))
	float PreviewDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Photo", meta=(ClampMin="1.0"))
	float TransferRequestTimeout = 10.0f;

	UFUNCTION(BlueprintImplementableEvent, Category="Photo", meta=(DisplayName="On Photo Displayed"))
	void BP_OnPhotoDisplayed(UTexture* Photo);

private:
	UFUNCTION()
	void HandlePhotoEvidenceChanged();

	UFUNCTION()
	void HandleReceiveButtonClicked();

	UFUNCTION()
	void HandlePhotoTextureReceived(FGuid PhotoId, UTexture2D* Texture);

	void RefreshAvailablePhotos();
	void SetTransferNotificationVisible(bool bVisible);
	void HandleTransferRequestTimeout();

	UPROPERTY(Transient)
	TObjectPtr<UTexture> DisplayedPhoto;

	UPROPERTY(Transient)
	TObjectPtr<ANoPhotosGameState> ObservedGameState;

	UPROPERTY(Transient)
	TObjectPtr<UNPPhotoTransferComponent> TransferComponent;

	TSet<FGuid> KnownPhotoIds;
	TArray<FGuid> AvailablePhotoIds;
	FGuid PendingPhotoId;
	FTimerHandle PreviewHideTimer;
	FTimerHandle TransferRequestTimer;
};
