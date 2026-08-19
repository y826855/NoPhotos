#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPLobbyWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class NOPHOTOS_API UNPLobbyWidget : public UNPUserWidget
{
	GENERATED_BODY()

public:
	UNPLobbyWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LeaveButton;

	UFUNCTION()
	void OnStartButtonClicked();
	UFUNCTION()
	void OnLeaveClicked();
};