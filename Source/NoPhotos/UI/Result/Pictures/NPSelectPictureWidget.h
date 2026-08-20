#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPSelectPictureWidget.generated.h"

class UButton;
class UTextBlock;
class UTexture2D;
class UNPPictureList;
class UNPShowPicture;

UCLASS()
class NOPHOTOS_API UNPSelectPictureWidget : public UNPUserWidget
{
	GENERATED_BODY()

public:
	//테스트용 사진 배열
	UFUNCTION(BlueprintCallable, Category = "Picture")
	void InitializePictures(const TArray<UTexture2D*>& InPictures);

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Picture")
	void BP_OnSelectionConfirmed(const TArray<int32>& SelectedPictureIndices);

private:
	UFUNCTION()
	void HandlePictureClicked(int32 PictureIndex);
	UFUNCTION()
	void HandleSelectRequested(int32 PictureIndex);
	UFUNCTION()
	void HandleNextButtonClicked();

	//테스트용 사진 추가
	void InitializeSamplePictures();
	
	void ShowPicture(int32 PictureIndex);
	void UpdateSelectedPictureCountText();
	TArray<int32> GetSelectedPictureIndices() const;

	UPROPERTY(EditDefaultsOnly, Category = "Picture", meta=(ClampMin = "1"))
	int32 MaxSelectedPictureCount = 5;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNPShowPicture> ShowPictureWidget;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNPPictureList> PictureListWidget;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SelectedPictureCountText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NextButton;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> PictureTextures;

	UPROPERTY(Transient)
	int32 CurrentPictureIndex = INDEX_NONE;
};