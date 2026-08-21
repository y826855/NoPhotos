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

void UNPRoomItemWidget::SetupRoomInfo(
	const int32 InRoomNumber,
	const FString& HostName,
	const int32 CurrentPlayers,
	const int32 MaxPlayers)
{
	RoomNumber = InRoomNumber;

	if (IsValid(RoomNameText))
	{
		const FString RoomTitle = HostName.IsEmpty()
			? FString::Printf(TEXT("방 #%d"), RoomNumber)
			: FString::Printf(TEXT("%s님의 방"), *HostName);
		RoomNameText->SetText(FText::FromString(RoomTitle));
	}

	if (IsValid(HostNameText))
	{
		HostNameText->SetText(FText::FromString(HostName));
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
