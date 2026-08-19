#include "UI/MainMenu/NPLobbyWidget.h"
#include "Components/Button.h"
#include "Core/NPPlayerController.h"

UNPLobbyWidget::UNPLobbyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNPLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer());
	const bool bIsRoomHost = IsValid(NPPC) && NPPC->IsRoomHost();

	if (IsValid(StartButton))
	{
		StartButton->SetVisibility(bIsRoomHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bIsRoomHost)
		{
			StartButton->OnClicked.AddDynamic(this, &UNPLobbyWidget::OnStartButtonClicked);
		}
	}

	if (IsValid(LeaveButton))
	{
		LeaveButton->OnClicked.AddDynamic(this, &UNPLobbyWidget::OnLeaveClicked);
	}
}

void UNPLobbyWidget::NativeDestruct()
{
	if (IsValid(StartButton))
	{
		StartButton->OnClicked.RemoveAll(this);
	}

	if (IsValid(LeaveButton))
	{
		LeaveButton->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UNPLobbyWidget::OnStartButtonClicked()
{
	ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer());
	if (!IsValid(NPPC))
	{
		return;
	}
	
	if (NPPC->IsRoomHost())
	{
		NPPC->RequestStartGame();
	}
}

void UNPLobbyWidget::OnLeaveClicked()
{
	if (ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer()))
	{
		NPPC->ExitRoom();
	}
}
