#include "NPRoomLog.h"

#include "NPRoomSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogNPRoom);

namespace NPRoomLog
{
	FString GetNetModeLabel(const UObject* WorldContextObject)
	{
		const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
		if (!World)
		{
			return TEXT("NoWorld");
		}

		switch (World->GetNetMode())
		{
		case NM_Standalone:
			return TEXT("Standalone");
		case NM_DedicatedServer:
			return TEXT("DedicatedServer");
		case NM_ListenServer:
			return TEXT("ListenServer");
		case NM_Client:
			return TEXT("Client");
		default:
			return TEXT("Unknown");
		}
	}

	FString BuildMessage(const UObject* WorldContextObject, const FString& Message)
	{
		return FString::Printf(TEXT("[Room][%s] %s"), *GetNetModeLabel(WorldContextObject), *Message);
	}

	void DisplayOnLocalPlayer(const UObject* WorldContextObject, const FString& Message, const FLinearColor& Color)
	{
		UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
		if (!RoomSubsystem)
		{
			return;
		}

		RoomSubsystem->DisplayDebugMessage(Message, Color);
	}

	void Info(const UObject* WorldContextObject, const FString& Message)
	{
		const FString FinalMessage = BuildMessage(WorldContextObject, Message);
		UE_LOG(LogNPRoom, Log, TEXT("%s"), *FinalMessage);

		DisplayOnLocalPlayer(WorldContextObject, FinalMessage, FLinearColor::Green);
	}

	void Warning(const UObject* WorldContextObject, const FString& Message)
	{
		const FString FinalMessage = BuildMessage(WorldContextObject, Message);
		UE_LOG(LogNPRoom, Warning, TEXT("%s"), *FinalMessage);

		DisplayOnLocalPlayer(WorldContextObject, FinalMessage, FLinearColor::Yellow);
	}
}
