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

	//방 상태나 플레이어 Ready 상태 변화 시 UI 갱신
	UFUNCTION()
	void OnRoomStateChanged();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ActionButton;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActionButtonText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LeaveButton;

	UFUNCTION()
	void OnActionButtonClicked();
	UFUNCTION()
	void OnLeaveClicked();
	void RefreshLobbyUI();
};