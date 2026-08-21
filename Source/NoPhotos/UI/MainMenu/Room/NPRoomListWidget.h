#pragma once

#include "CoreMinimal.h"
#include "UI/NPUserWidget.h"
#include "NPRoomListWidget.generated.h"

class UScrollBox;
class UButton;
class UNPRoomItemWidget;

UCLASS()
class NOPHOTOS_API UNPRoomListWidget : public UNPUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 방 목록을 스크롤 박스에 동적으로 채움
	UFUNCTION(BlueprintCallable, Category = "Room")
	void RefreshRoomList();

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> RoomListScrollBox;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RefreshButton;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	// 방 항목 위젯 BPP 클래스 (WBP_RoomItem 지정)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPRoomItemWidget> RoomItemClass;

	UFUNCTION()
	void OnRefreshClicked();
	UFUNCTION()
	void OnCloseClicked();
	UFUNCTION()
	void OnFindRoomsComplete(const TArray<int32>& RoomIndices);
	// 특정 방 항목을 클릭했을 때 호출될 함수
	UFUNCTION()
	void OnRoomItemSelected(int32 SelectedRoomNumber);
};
