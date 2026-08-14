#include "NPRoomSubsystem.h"

#include "NPRoomLog.h"
#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"

void UNPRoomSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	DebugDrawHandle = UDebugDrawService::Register(
		TEXT("Game"),
		FDebugDrawDelegate::CreateUObject(this, &UNPRoomSubsystem::DrawDebugMessages));

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UNPRoomSubsystem::HandleNetworkFailure);
	}
}

void UNPRoomSubsystem::Deinitialize()
{
	if (DebugDrawHandle.IsValid())
	{
		UDebugDrawService::Unregister(DebugDrawHandle);
		DebugDrawHandle.Reset();
	}

	if (GEngine && NetworkFailureHandle.IsValid())
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		NetworkFailureHandle.Reset();
	}

	DebugMessages.Reset();
	Super::Deinitialize();
}

bool UNPRoomSubsystem::HostRoom()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		NPRoomLog::Warning(this, TEXT("호스트 생성 실패: GameInstance를 찾지 못했습니다."));
		return false;
	}

	if (const UWorld* World = GameInstance->GetWorld(); World && World->GetNetMode() == NM_ListenServer)
	{
		NPRoomLog::Warning(this, TEXT("호스트 생성 생략: 현재 월드는 이미 ListenServer입니다."));
		return false;
	}

	NPRoomLog::Info(this, TEXT("호스트 생성 요청: 현재 메뉴 월드에서 ListenPort=7777 활성화"));
	if (!GameInstance->EnableListenServer(true, 7777))
	{
		NPRoomLog::Warning(this, TEXT("호스트 생성 실패: Listen Server를 활성화하지 못했습니다."));
		return false;
	}

	NPRoomLog::Info(this, TEXT("호스트 생성 완료: 127.0.0.1:7777에서 게스트 접속 대기"));
	return true;
}

bool UNPRoomSubsystem::JoinLocalRoom()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (const UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr; World && World->GetNetMode() == NM_Client)
	{
		NPRoomLog::Warning(this, TEXT("방 참가 생략: 현재 월드는 이미 서버에 연결된 Client입니다."));
		return false;
	}

	APlayerController* PlayerController = GameInstance ? GameInstance->GetFirstLocalPlayerController() : nullptr;
	if (!PlayerController)
	{
		NPRoomLog::Warning(this, TEXT("방 참가 실패: 로컬 PlayerController를 찾지 못했습니다."));
		return false;
	}

	NPRoomLog::Info(this, TEXT("로컬 방 참가 요청: 127.0.0.1:7777"));
	PlayerController->ClientTravel(TEXT("127.0.0.1:7777"), TRAVEL_Absolute);
	return true;
}

void UNPRoomSubsystem::DisplayDebugMessage(const FString& Message, const FLinearColor& Color)
{
	FNPRoomDebugMessage& DebugMessage = DebugMessages.AddDefaulted_GetRef();
	DebugMessage.Message = Message;
	DebugMessage.Color = Color;
	DebugMessage.ExpirationTime = FPlatformTime::Seconds() + 8.0;
}

void UNPRoomSubsystem::DrawDebugMessages(UCanvas* Canvas, APlayerController* PlayerController)
{
	if (!Canvas || !PlayerController || PlayerController->GetGameInstance() != GetGameInstance() || !GEngine)
	{
		return;
	}

	const double CurrentTime = FPlatformTime::Seconds();
	DebugMessages.RemoveAll(
		[CurrentTime](const FNPRoomDebugMessage& DebugMessage)
		{
			return DebugMessage.ExpirationTime <= CurrentTime;
		});

	float ScreenY = 50.0f;
	for (const FNPRoomDebugMessage& DebugMessage : DebugMessages)
	{
		Canvas->SetDrawColor(DebugMessage.Color.ToFColor(true));
		Canvas->DrawText(GEngine->GetSmallFont(), DebugMessage.Message, 40.0f, ScreenY);
		ScreenY += 20.0f;
	}
}

void UNPRoomSubsystem::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	const ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	UWorld* FailureWorld = World ? World : NetDriver ? NetDriver->GetWorld() : nullptr;
	if (!FailureWorld || FailureWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	FString FailureMessage;
	switch (FailureType)
	{
	case ENetworkFailure::ConnectionTimeout:
		FailureMessage = TEXT("접속 실패: 서버 응답 시간이 초과되었습니다.");
		break;
	case ENetworkFailure::PendingConnectionFailure:
		FailureMessage = TEXT("접속 실패: 서버에 연결할 수 없습니다.");
		break;
	case ENetworkFailure::ConnectionLost:
		FailureMessage = TEXT("서버와의 연결이 끊어졌습니다.");
		break;
	case ENetworkFailure::NetDriverCreateFailure:
		FailureMessage = TEXT("네트워크 초기화 실패: NetDriver를 생성하지 못했습니다.");
		break;
	case ENetworkFailure::NetDriverListenFailure:
		FailureMessage = TEXT("방 생성 실패: 서버 포트를 열지 못했습니다.");
		break;
	default:
		FailureMessage = FString::Printf(
			TEXT("네트워크 오류: %s"),
			ENetworkFailure::ToString(FailureType));
		break;
	}

	if (!ErrorString.IsEmpty())
	{
		FailureMessage += FString::Printf(TEXT(" (%s)"), *ErrorString);
	}

	NPRoomLog::Warning(this, FailureMessage);
}
