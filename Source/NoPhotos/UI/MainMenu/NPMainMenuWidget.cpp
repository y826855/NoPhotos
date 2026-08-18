#include "UI/MainMenu/NPMainMenuWidget.h"
#include "Components/Button.h"
#include "Core/NPPlayerController.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SubSystem/NPUIManagerSubsystem.h"

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
	// 방 목록 UI(WBP_RoomList)를 화면 위에 PopUp 형태로 띄움
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UNPUIManagerSubsystem* UIManager = GI->GetSubsystem<UNPUIManagerSubsystem>())
		{
			if (IsValid(RoomListWidgetClass))
			{
				UIManager->PushWidget(RoomListWidgetClass);
			}
		}
	}
}

void UNPMainMenuWidget::OnExitClicked()
{
	APlayerController* PC = GetOwningPlayer();
	UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
}
