#include "UI/MainMenu/NPLobbyWidget.h"

#include "Components/Button.h"
#include "Core/NPPlayerController.h"
#include "Core/Room/NPRoomGameState.h"

UNPLobbyWidget::UNPLobbyWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UNPLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(StartButton))
	{
		StartButton->OnClicked.AddDynamic(this, &UNPLobbyWidget::OnStartButtonClicked);
	}

	if (IsValid(LeaveButton))
	{
		LeaveButton->OnClicked.AddDynamic(this, &UNPLobbyWidget::OnLeaveClicked);
	}

	ANPRoomGameState* RoomGameState = GetWorld() ? GetWorld()->GetGameState<ANPRoomGameState>()	: nullptr;

	if (IsValid(RoomGameState))
	{
		BoundRoomGameState = RoomGameState;
		RoomGameState->OnRoomStateChanged.AddDynamic(this, &UNPLobbyWidget::OnRoomStateChanged);
	}

	RefreshStartButtonVisibility();
}

void UNPLobbyWidget::NativeDestruct()
{
	if (BoundRoomGameState.IsValid())
	{
		BoundRoomGameState->OnRoomStateChanged.RemoveDynamic(this, &UNPLobbyWidget::OnRoomStateChanged);
	}

	if (IsValid(StartButton))
	{
		StartButton->OnClicked.RemoveAll(this);
	}

	if (IsValid(LeaveButton))
	{
		LeaveButton->OnClicked.RemoveAll(this);
	}

	BoundRoomGameState.Reset();

	Super::NativeDestruct();
}

void UNPLobbyWidget::RefreshStartButtonVisibility()
{
	if (!IsValid(StartButton))
	{
		return;
	}

	ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer());
	const bool bIsRoomHost = IsValid(NPPC) && NPPC->IsRoomHost();
	const bool bCanStartGame = BoundRoomGameState.IsValid()	&& BoundRoomGameState->CanHostStartGame();

	StartButton->SetVisibility(bIsRoomHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	StartButton->SetIsEnabled(bIsRoomHost && bCanStartGame);
}

void UNPLobbyWidget::OnRoomStateChanged()
{
	RefreshStartButtonVisibility();
}

void UNPLobbyWidget::OnStartButtonClicked()
{
	ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer());
	if (IsValid(NPPC) && NPPC->IsRoomHost() && BoundRoomGameState.IsValid() && BoundRoomGameState->CanHostStartGame())
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
