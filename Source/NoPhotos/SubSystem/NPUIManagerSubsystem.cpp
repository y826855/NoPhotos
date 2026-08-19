#include "SubSystem/NPUIManagerSubsystem.h"
#include "UI/NPUserWidget.h"

UNPUIManagerSubsystem::UNPUIManagerSubsystem()
{
	
}

void UNPUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UNPUIManagerSubsystem::Deinitialize()
{
	PopAllWidgets();
	Super::Deinitialize();
}

UNPUserWidget* UNPUIManagerSubsystem::PushWidget(TSubclassOf<UNPUserWidget> WidgetClass, int32 ZOrder)
{
	if (!WidgetClass)
	{
		return nullptr;
	}

	CleanInvalidWidgetsFromStack();
	if (UNPUserWidget* TopWidget = GetTopWidget();
		IsValid(TopWidget) && TopWidget->IsA(WidgetClass))
	{
		if (!TopWidget->IsInViewport())
		{
			TopWidget->AddToViewport(ZOrder);
		}
		return TopWidget;
	}

	APlayerController* OwningPlayer = GetOwningPlayerController();
	if (!IsValid(OwningPlayer))
	{
		return nullptr;
	}

	UNPUserWidget* Widget = CreateWidget<UNPUserWidget>(OwningPlayer, WidgetClass);
	if (!IsValid(Widget))
	{
		return nullptr;
	}

	Widget->AddToViewport(ZOrder);
	WidgetStack.Add(Widget);
	Widget->OnPushed();
	ApplyInputModeForTopWidget();

	return Widget;
}

bool UNPUIManagerSubsystem::RequestPopWidget()
{
	CleanInvalidWidgetsFromStack();

	UNPUserWidget* Widget = GetTopWidget();
	if (!IsValid(Widget) || Widget->IsPopRequested())
	{
		return false;
	}

	Widget->SetPopRequested(true);
	Widget->OnPopRequested();

	return true;
}

bool UNPUIManagerSubsystem::CompletePopWidget(UNPUserWidget* Widget)
{
	return RemoveWidgetFromStack(Widget, false);
}

void UNPUIManagerSubsystem::PopAllWidgets()
{
	while (UNPUserWidget* TopWidget = GetTopWidget())
	{
		RemoveWidgetFromStack(TopWidget, true);
	}
	CleanInvalidWidgetsFromStack();
	ApplyInputModeForTopWidget();
}

//가장 위에 있는 위젯 찾기
UNPUserWidget* UNPUIManagerSubsystem::GetTopWidget() const
{
	for (int32 WidgetIndex = WidgetStack.Num() - 1; WidgetIndex >= 0; --WidgetIndex)
	{
		UNPUserWidget* Widget = WidgetStack[WidgetIndex].Get();
		if (IsValid(Widget))
		{
			return Widget;
		}
	}
	return nullptr;
}

//위젯을 제거, 입력 모드 갱신
bool UNPUIManagerSubsystem::RemoveWidgetFromStack(UNPUserWidget* Widget, bool bForceRemove)
{
	if (!IsValid(Widget))
	{
		return false;
	}

	const int32 WidgetIndex = WidgetStack.IndexOfByPredicate(
		[Widget](const TObjectPtr<UNPUserWidget>& StackWidget)
		{
			return StackWidget.Get() == Widget;
		});
	if (WidgetIndex == INDEX_NONE)
	{
		return false;
	}

	const bool bIsTopWidget = WidgetIndex == WidgetStack.Num() - 1;
	if (!bForceRemove && !bIsTopWidget && !Widget->IsPopRequested())
	{
		return false;
	}

	WidgetStack.RemoveAt(WidgetIndex);
	Widget->SetPopRequested(false);
	Widget->OnPopped();
	Widget->RemoveFromParent();
	ApplyInputModeForTopWidget();

	return true;
}

// 현재 이 GameInstance를 실행 중인 로컬 플레이어의 PlayerController를 반환한다.
APlayerController* UNPUIManagerSubsystem::GetOwningPlayerController() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return IsValid(GameInstance) ? GameInstance->GetFirstLocalPlayerController() : nullptr;
}

// 열린 화면이 없으면 게임 입력으로, 있으면 최상단 위젯의 설정에 맞는 UI 입력으로 전환한다.
void UNPUIManagerSubsystem::ApplyInputModeForTopWidget()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController))
	{
		return;
	}

	UNPUserWidget* TopWidget = GetTopWidget();
	if (!IsValid(TopWidget))
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = false;
		return;
	}

	if (TopWidget->UsesGameOnlyInputMode())
	{
		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = false;
		return;
	}

	PlayerController->bShowMouseCursor = TopWidget->ShouldShowMouseCursor();
	if (TopWidget->UsesGameAndUIInputMode())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(TopWidget->TakeWidget());
		PlayerController->SetInputMode(InputMode);
		return;
	}

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TopWidget->TakeWidget());
	PlayerController->SetInputMode(InputMode);
}

void UNPUIManagerSubsystem::CleanInvalidWidgetsFromStack()
{
	for (int32 WidgetIndex = WidgetStack.Num() - 1; WidgetIndex >= 0; --WidgetIndex)
	{
		if (!IsValid(WidgetStack[WidgetIndex].Get()))
		{
			WidgetStack.RemoveAt(WidgetIndex);
		}
	}
}
