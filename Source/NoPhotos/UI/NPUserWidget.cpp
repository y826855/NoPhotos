#include "UI/NPUserWidget.h"
#include "SubSystem/NPUIManagerSubsystem.h"

void UNPUserWidget::OnPushed_Implementation()
{
}

void UNPUserWidget::OnPopRequested_Implementation()
{
	CompletePop();
}

void UNPUserWidget::OnPopped_Implementation()
{
	
}

bool UNPUserWidget::CompletePop()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return false;
	}

	UNPUIManagerSubsystem* UIManagerSubsystem = GameInstance->GetSubsystem<UNPUIManagerSubsystem>();
	if (!IsValid(UIManagerSubsystem))
	{
		return false;
	}

	return UIManagerSubsystem->CompletePopWidget(this);
}
