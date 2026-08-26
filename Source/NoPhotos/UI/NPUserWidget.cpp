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

void UNPUserWidget::SetInputModeState(const ENPWidgetInputMode InInputMode)
{
	bUseGameAndUIInputMode = InInputMode == ENPWidgetInputMode::GameAndUI;
	bUseGameOnlyInputMode = InInputMode == ENPWidgetInputMode::GameOnly;
	bShowMouseCursor = InInputMode != ENPWidgetInputMode::GameOnly;
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
