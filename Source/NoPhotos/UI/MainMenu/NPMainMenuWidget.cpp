#include "UI/MainMenu/NPMainMenuWidget.h"
#include "Components/Button.h"
#include "Core/NPPlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

UNPMainMenuWidget::UNPMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void UNPMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (IsValid(HostButton))
	{
		HostButton->OnClicked.AddDynamic(this, &UNPMainMenuWidget::OnHostGameClicked);
	}

	if (IsValid(JoinButton))
	{
		JoinButton->OnClicked.AddDynamic(this, &UNPMainMenuWidget::OnJoinGameClicked);
	}

	if (IsValid(ExitButton))
	{
		ExitButton->OnClicked.AddDynamic(this, &UNPMainMenuWidget::OnExitClicked);
	}
}

void UNPMainMenuWidget::NativeDestruct()
{
	Super::NativeDestruct();
	
	if (IsValid(HostButton))
	{
		HostButton->OnClicked.RemoveAll(this);
	}

	if (IsValid(JoinButton))
	{
		JoinButton->OnClicked.RemoveAll(this);
	}

	if (IsValid(ExitButton))
	{
		ExitButton->OnClicked.RemoveAll(this);
	}
}

void UNPMainMenuWidget::OnHostGameClicked()
{
	if (ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer()))
	{
		NPPC->HostRoom();
	}
}

void UNPMainMenuWidget::OnJoinGameClicked()
{
	if (ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer()))
	{
		// 기본적으로 1번 방으로 참가 요청 (이후 방 목록 추가하면서 선택 할 수 있게 늘릴 예정...)
		const int32 TargetRoomNumber = 1; 
		const bool bSuccess = NPPC->JoinRoom(TargetRoomNumber);
	}
}

void UNPMainMenuWidget::OnExitClicked()
{
	APlayerController* PC = GetOwningPlayer();
	UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
}
