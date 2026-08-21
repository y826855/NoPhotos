#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPResultWidget.generated.h"

class UButton;

UCLASS()
class NOPHOTOS_API UNPResultWidget : public UNPUserWidget
{
	GENERATED_BODY()
	
public:
	UNPResultWidget(const FObjectInitializer& ObjectInitializer);
	
protected:    	
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
    
private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RetryButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ExitButton;
    
	UFUNCTION()
	void OnRetryClicked();
	UFUNCTION()
	void OnExitClicked();
};
