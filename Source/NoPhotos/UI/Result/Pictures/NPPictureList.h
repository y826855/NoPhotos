//사진 리스트

#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPPictureList.generated.h"

class UScrollBox;
class UNPPictureWidget;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNPPictureListItemClicked,	int32,	PictureIndex);
UCLASS()
class NOPHOTOS_API UNPPictureList : public UNPUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Picture")
    FNPPictureListItemClicked OnPictureClicked;
	
	void AddPicture(UTexture2D* InTexture);
	void SetPictureSelected(int32 PictureIndex, bool bSelected);
	void ClearPictures();
	bool IsPictureSelected(int32 PictureIndex) const;
	int32 GetSelectedPictureCount() const;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly, Category = "Picture")
	TSubclassOf<UNPPictureWidget> PictureWidgetClass;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> PictureScrollBox;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNPPictureWidget>> PictureWidgets;
	UPROPERTY(Transient)
	TArray<bool> SelectedPictures;
	
	UFUNCTION()
	void HandlePictureWidgetClicked(UNPPictureWidget* ClickedPictureWidget);
};