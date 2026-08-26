#include "UI/MainMenu/NPLobbyWidget.h"

#include "Components/Button.h"
#include "Core/Room/NPRoomGameState.h"
#include "Core/Room/NPRoomPlayerController.h"

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

	ANPRoomPlayerController* RoomPlayerController = Cast<ANPRoomPlayerController>(GetOwningPlayer());
	const bool bIsRoomHost = IsValid(RoomPlayerController) && RoomPlayerController->IsRoomHost();
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
	ANPRoomPlayerController* RoomPlayerController = Cast<ANPRoomPlayerController>(GetOwningPlayer());
	if (IsValid(RoomPlayerController) && RoomPlayerController->IsRoomHost()
		&& BoundRoomGameState.IsValid() && BoundRoomGameState->CanHostStartGame())
	{
		RoomPlayerController->RequestStartGame();
	}
}

void UNPLobbyWidget::OnLeaveClicked()
{
	if (ANPRoomPlayerController* RoomPlayerController = Cast<ANPRoomPlayerController>(GetOwningPlayer()))
	{
		RoomPlayerController->ExitRoom();
	}
}
