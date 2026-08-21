#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPRoomItemWidget.generated.h"

class UTextBlock;
class UButton;

// 방 선택 시 해당 방 번호를 전달할 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomSelectedDelegate, int32, RoomNumber);

UCLASS()
class NOPHOTOS_API UNPRoomItemWidget : public UNPUserWidget
{
	GENERATED_BODY()

public:
	// 방 정보(방 번호, 현재 인원, 최대 인원) 데이터 세팅
	void SetupRoomInfo(int32 InRoomNumber, int32 CurrentPlayers, int32 MaxPlayers);

	FOnRoomSelectedDelegate OnRoomSelected;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RoomNameText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerCountText;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SelectButton;

	UFUNCTION()
	void OnSelectButtonClicked();

	int32 RoomNumber = 0;
};