#include "UI/Result/NPResultWidget.h"

#include "Components/Button.h"
#include "Core/Main/NPMainPlayerController.h"

UNPResultWidget::UNPResultWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNPResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ANPMainPlayerController* NPPC = Cast<ANPMainPlayerController>(GetOwningPlayer());
	const bool bIsRoomHost = IsValid(NPPC) && NPPC->IsListenServerHost();

	if (IsValid(RetryButton))
	{
		RetryButton->SetVisibility(
			bIsRoomHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		if (bIsRoomHost)
		{
			RetryButton->OnClicked.AddDynamic(this, &UNPResultWidget::OnRetryClicked);
		}
	}

	if (IsValid(ExitButton))
	{
		ExitButton->OnClicked.AddDynamic(this, &UNPResultWidget::OnExitClicked);
	}
}

void UNPResultWidget::NativeDestruct()
{
	if (IsValid(RetryButton))
	{
		RetryButton->OnClicked.RemoveAll(this);
	}

	if (IsValid(ExitButton))
	{
		ExitButton->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UNPResultWidget::OnRetryClicked()
{
	ANPMainPlayerController* NPPC = Cast<ANPMainPlayerController>(GetOwningPlayer());
	if (!IsValid(NPPC) || !NPPC->IsListenServerHost())
	{
		return;
	}

	NPPC->RequestRestartRoom();
}

void UNPResultWidget::OnExitClicked()
{
	if (ANPMainPlayerController* NPPC=Cast<ANPMainPlayerController>(GetOwningPlayer()))
	{
		NPPC->ExitToMainMenu();
	}
}