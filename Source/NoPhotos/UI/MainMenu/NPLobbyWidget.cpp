#include "UI/MainMenu/NPLobbyWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/NPPlayerController.h"
#include "Core/NPGameState.h"

UNPLobbyWidget::UNPLobbyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNPLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(ActionButton))
	{
		ActionButton->OnClicked.AddDynamic(this, &UNPLobbyWidget::OnActionButtonClicked);
	}

	if (IsValid(LeaveButton))
	{
		LeaveButton->OnClicked.AddDynamic(this, &UNPLobbyWidget::OnLeaveClicked);
	}

	// GameState 이벤트 바인딩 (방 상태 변경 시 UI 갱신)
	if (UWorld* World = GetWorld())
	{
		if (ANPGameState* GS = World->GetGameState<ANPGameState>())
		{
			GS->OnRoomStateChanged.AddDynamic(this, &UNPLobbyWidget::OnRoomStateChanged);
		}
	}

	RefreshLobbyUI();
}

void UNPLobbyWidget::NativeDestruct()
{
	if (IsValid(ActionButton))
	{
		ActionButton->OnClicked.RemoveAll(this);
	}

	if (IsValid(LeaveButton))
	{
		LeaveButton->OnClicked.RemoveAll(this);
	}

	if (UWorld* World = GetWorld())
	{
		if (ANPGameState* GS = World->GetGameState<ANPGameState>())
		{
			GS->OnRoomStateChanged.RemoveAll(this);
		}
	}

	Super::NativeDestruct();
}

void UNPLobbyWidget::OnActionButtonClicked()
{
	ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer());
	if (!IsValid(NPPC))
	{
		return;
	}

	// 1. 방장(호스트)인 경우 -> 게임 시작 요청
	if (NPPC->IsRoomHost())
	{
		NPPC->RequestStartGame();
	}
	// 2. 게스트인 경우 -> 현재 준비 상태의 반댓값으로 토글 요청
	else
	{
		const bool bCurrentReady = NPPC->IsRoomReady();
		NPPC->SetReady(!bCurrentReady);
	}
}

void UNPLobbyWidget::OnLeaveClicked()
{
	if (ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer()))
	{
		// 방 나가기 요청 (호스트/게스트 내부 분기 처리됨)
		NPPC->ExitRoom();
	}
}

void UNPLobbyWidget::OnRoomStateChanged()
{
	RefreshLobbyUI();
}

void UNPLobbyWidget::RefreshLobbyUI()
{
	ANPPlayerController* NPPC = Cast<ANPPlayerController>(GetOwningPlayer());
	if (!IsValid(NPPC) || !IsValid(ActionButton))
	{
		return;
	}

	// 호스트 여부 체크
	const bool bIsHost = NPPC->IsRoomHost();

	if (bIsHost)
	{
		// --- 호스트(방장) 모드 ---
		if (IsValid(ActionButtonText))
		{
			ActionButtonText->SetText(FText::FromString(TEXT("게임 시작")));
		}

		// 모든 조건(게스트 전원 Ready 등)이 충족되었을 때만 시작 버튼 활성화
		ActionButton->SetIsEnabled(NPPC->CanStartGame());
	}
	else
	{
		// --- 게스트 모드 ---
		ActionButton->SetIsEnabled(true);

		// 현재 내 Ready 상태 확인 후 텍스트 전환
		const bool bIsReady = NPPC->IsRoomReady();
		if (IsValid(ActionButtonText))
		{
			ActionButtonText->SetText(bIsReady ? FText::FromString(TEXT("준비 취소")) : FText::FromString(TEXT("준비 완료")));
		}
	}
}