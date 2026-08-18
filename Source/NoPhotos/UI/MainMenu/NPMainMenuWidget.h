#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPMainMenuWidget.generated.h"

class UButton;

UCLASS()
class NOPHOTOS_API UNPMainMenuWidget : public UNPUserWidget
{
	GENERATED_BODY()
	
public:
	UNPMainMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HostButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> JoinButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ExitButton;

	UFUNCTION()
	void OnHostGameClicked();
	UFUNCTION()
	void OnJoinGameClicked();
	UFUNCTION()
	void OnExitClicked();
};
