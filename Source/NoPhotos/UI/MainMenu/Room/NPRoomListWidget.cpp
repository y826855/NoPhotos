#include "UI/MainMenu/Room/NPRoomListWidget.h"
#include "UI/MainMenu/Room/NPRoomItemWidget.h"

#include "Components/ScrollBox.h"
#include "Components/Button.h"
#include "Core/NPPlayerController.h"
#include "Core/Room/NPRoomSubsystem.h"
#include "SubSystem/NPUIManagerSubsystem.h"

void UNPRoomListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(RefreshButton))
	{
		RefreshButton->OnClicked.AddDynamic(this, &UNPRoomListWidget::OnRefreshClicked);
	}

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.AddDynamic(this, &UNPRoomListWidget::OnCloseClicked);
	}

	// UNPRoomSubsystem의 방 검색 완료 델리게이트 바인딩
	if (UGameInstance* GI = GetGameInstance())
    	{
    		if (UNPRoomSubsystem* RoomSubsystem = GI->GetSubsystem<UNPRoomSubsystem>())
    		{
    			RoomSubsystem->OnFindRoomsComplete.AddDynamic(this, &UNPRoomListWidget::OnFindRoomsComplete);
    		}
    	}
    
    	// 방 목록 UI가 켜질 때 자동으로 첫 검색 수행
    	OnRefreshClicked();
}

void UNPRoomListWidget::NativeDestruct()
{
	if (IsValid(RefreshButton))
	{
		RefreshButton->OnClicked.RemoveAll(this);
	}

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.RemoveAll(this);
	}
	
	if (UGameInstance* GI = GetGameInstance())
    {
    	if (UNPRoomSubsystem* RoomSubsystem = GI->GetSubsystem<UNPRoomSubsystem>())
    	{
    		RoomSubsystem->OnFindRoomsComplete.RemoveAll(this);
    	}
    }

	Super::NativeDestruct();
}

void UNPRoomListWidget::OnRefreshClicked()
{
	if (ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer()))
	{
		NPPC->FindRooms();
	}
}

void UNPRoomListWidget::OnFindRoomsComplete(const TArray<int32>& RoomIndices)
{
	RefreshRoomList();
}

void UNPRoomListWidget::RefreshRoomList()
{
	if (!IsValid(RoomListScrollBox) || !IsValid(RoomItemClass)) return;

	RoomListScrollBox->ClearChildren();

	UGameInstance* GI = GetGameInstance();
	UNPRoomSubsystem* RoomSubsystem = GI ? GI->GetSubsystem<UNPRoomSubsystem>() : nullptr;
	if (!IsValid(RoomSubsystem)) return;

	const TArray<FNPRoomListEntry> Rooms = RoomSubsystem->GetListedRooms();
	for (const FNPRoomListEntry& Room : Rooms)
	{
		UNPRoomItemWidget* ItemWidget = CreateWidget<UNPRoomItemWidget>(this, RoomItemClass);
		if (IsValid(ItemWidget))
		{
			ItemWidget->SetupRoomInfo(
				Room.RoomNumber,
				Room.HostName,
				Room.CurrentPlayers,
				Room.MaxPlayers);
			ItemWidget->OnRoomSelected.AddDynamic(this, &UNPRoomListWidget::OnRoomItemSelected);
			RoomListScrollBox->AddChild(ItemWidget);
		}
	}
}

void UNPRoomListWidget::OnRoomItemSelected(int32 SelectedRoomNumber)
{
	if (ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer()))
	{
		NPPC->JoinRoom(SelectedRoomNumber);
	}
}

void UNPRoomListWidget::OnCloseClicked()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UNPUIManagerSubsystem* UIManager = GI->GetSubsystem<UNPUIManagerSubsystem>())
		{
			UIManager->RequestPopWidget();
		}
	}
}
