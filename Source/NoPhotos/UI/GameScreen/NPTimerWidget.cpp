#include "UI/GameScreen/NPTimerWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

UNPTimerWidget::UNPTimerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNPTimerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//서버시간 추가되면 연동 예정...
	RemainingTimeSeconds = 300;
	UpdateTimerUI();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CountdownTimerHandle,
			this,
			&UNPTimerWidget::UpdateTimerUI,
			1.0f,
			true
		);
	}
}

void UNPTimerWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CountdownTimerHandle);
	}

	Super::NativeDestruct();
}

void UNPTimerWidget::UpdateTimerUI()
{
	if (RemainingTimeSeconds < 0)
	{
		RemainingTimeSeconds = 0;
	}

	const int32 Minutes = RemainingTimeSeconds / 60;
	const int32 Seconds = RemainingTimeSeconds % 60;

	if (IsValid(MinuteText))
	{
		MinuteText->SetText(FText::FromString(FString::Printf(TEXT("%2d"), Minutes)));
	}

	if (IsValid(SecondText))
	{
		SecondText->SetText(FText::FromString(FString::Printf(TEXT("%02d"), Seconds)));
	}

	if (RemainingTimeSeconds > 0)
	{
		--RemainingTimeSeconds;
	}
}

