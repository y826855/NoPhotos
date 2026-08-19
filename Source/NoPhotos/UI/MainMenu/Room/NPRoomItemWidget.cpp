#include "UI/MainMenu/Room/NPRoomItemWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UNPRoomItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(SelectButton))
	{
		SelectButton->OnClicked.AddDynamic(this, &UNPRoomItemWidget::OnSelectButtonClicked);
	}
}

void UNPRoomItemWidget::NativeDestruct()
{
	if (IsValid(SelectButton))
	{
		SelectButton->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UNPRoomItemWidget::SetupRoomInfo(int32 InRoomNumber, int32 CurrentPlayers, int32 MaxPlayers)
{
	RoomNumber = InRoomNumber;

	if (IsValid(RoomNameText))
	{
		RoomNameText->SetText(FText::FromString(FString::Printf(TEXT("방 #%d"), RoomNumber)));
	}

	if (IsValid(PlayerCountText))
	{
		PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentPlayers, MaxPlayers)));
	}
}

void UNPRoomItemWidget::OnSelectButtonClicked()
{
	OnRoomSelected.Broadcast(RoomNumber);
}