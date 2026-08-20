#include "UI/Result/NPResultWidget.h"

#include "Components/Button.h"
#include "Core/NPPlayerController.h"

UNPResultWidget::UNPResultWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNPResultWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer());
	const bool bIsRoomHost = IsValid(NPPC) && NPPC->IsRoomHost();

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
	ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer());
	if (!IsValid(NPPC) || !NPPC->IsRoomHost())
	{
		return;
	}

	NPPC->RequestRestartRoom();
}

void UNPResultWidget::OnExitClicked()
{
	if (ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer()))
	{
		NPPC->ExitRoom();
	}
}
