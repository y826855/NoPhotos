#include "UI/MainMenu/Lobby/NPJoinPlayer.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void UNPJoinPlayer::SetupResult(const FString& InPlayerName)
{
	if (IsValid(PlayerNameText))
	{
		PlayerNameText->SetText(FText::FromString(InPlayerName));
	}
}

void UNPJoinPlayer::PlayJoinAnimation(const float Delay)
{
	if (!IsValid(JoinPoster))
	{
		return;
	}

	if (Delay <= 0.0f)
	{
		StartJoinAnimation();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			JoinAnimationTimer,
			this,
			&UNPJoinPlayer::StartJoinAnimation,
			Delay,
			false);
	}
}

void UNPJoinPlayer::PlayLeaveAnimation()
{
	if (bIsLeaving)
	{
		return;
	}

	bIsLeaving = true;
	SetupResult(TEXT("Unknown"));

	if (IsValid(LeavePoster))
	{
		PlayAnimation(LeavePoster);
		return;
	}

	HandleLeaveAnimationFinished();
}

void UNPJoinPlayer::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(LeavePoster))
	{
		FWidgetAnimationDynamicEvent LeaveAnimationFinishedDelegate;
		LeaveAnimationFinishedDelegate.BindDynamic(this, &UNPJoinPlayer::HandleLeaveAnimationFinished);
		BindToAnimationFinished(LeavePoster, LeaveAnimationFinishedDelegate);
	}
}

void UNPJoinPlayer::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(JoinAnimationTimer);
	}
	if (IsValid(LeavePoster))
	{
		UnbindAllFromAnimationFinished(LeavePoster);
	}

	Super::NativeDestruct();
}

void UNPJoinPlayer::StartJoinAnimation()
{
	if (IsValid(JoinPoster))
	{
		PlayAnimation(JoinPoster);
	}
}

void UNPJoinPlayer::HandleLeaveAnimationFinished()
{
	OnLeaveAnimationFinished.Broadcast(this);
}
