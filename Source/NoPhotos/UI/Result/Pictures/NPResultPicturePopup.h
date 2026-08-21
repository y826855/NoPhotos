#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPResultPicturePopup.generated.h"

class APlayerState;
class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
class UNPPhotoTransferComponent;
class UNPResultPicturePreviewPopup;

/**
 * WBP_ResultPicturePopup의 부모 클래스입니다.
 * 화면 배치는 블루프린트 Designer에서 만들고, 이 클래스는 사진 표시만 담당합니다.
 */
UCLASS()
class NOPHOTOS_API UNPResultPicturePopup : public UNPUserWidget
{
	GENERATED_BODY()

public:
	void OpenForPlayer(APlayerState* InPlayerState);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleThumbnail0Clicked();

	UFUNCTION()
	void HandleThumbnail1Clicked();

	UFUNCTION()
	void HandleThumbnail2Clicked();

	UFUNCTION()
	void HandleThumbnail3Clicked();

	UFUNCTION()
	void HandleThumbnail4Clicked();

	UFUNCTION()
	void HandlePhotoTextureReceived(FGuid PhotoId, UTexture2D* Texture);

	void HandleThumbnailClicked(int32 PictureIndex);
	void RequestNextPhoto();
	void ShowPicture(int32 PictureIndex);
	void UpdateStatusText(const FString& Text) const;

	// WBP에서 Is Variable을 켜고 이름을 정확히 맞춰야 합니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ThumbnailButton0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ThumbnailButton1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ThumbnailButton2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ThumbnailButton3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ThumbnailButton4;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ThumbnailImage0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ThumbnailImage1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ThumbnailImage2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ThumbnailImage3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ThumbnailImage4;

	// WBP_ResultPicturePreviewPopup을 지정합니다.
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPResultPicturePreviewPopup> PreviewPopupWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<APlayerState> ViewedPlayerState;

	UPROPERTY(Transient)
	TObjectPtr<UNPPhotoTransferComponent> TransferComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> PictureTextures;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> ThumbnailButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> ThumbnailImages;

	TArray<FGuid> PendingPhotoIds;
	FGuid DownloadingPhotoId;
};
