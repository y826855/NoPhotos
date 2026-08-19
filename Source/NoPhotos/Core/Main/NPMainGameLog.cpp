#include "NPMainGameLog.h"

#include "Core/Room/NPRoomSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogNPMainGame);

namespace NPMainGameLog
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

	void Info(const UObject* WorldContextObject, const FString& Message)
	{
		const FString FinalMessage = FString::Printf(
			TEXT("[MainGame][%s] %s"),
			*GetNetModeLabel(WorldContextObject),
			*Message);
		UE_LOG(LogNPMainGame, Log, TEXT("%s"), *FinalMessage);

		UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		UNPRoomSubsystem* RoomSubsystem = GameInstance ? GameInstance->GetSubsystem<UNPRoomSubsystem>() : nullptr;
		if (RoomSubsystem)
		{
			RoomSubsystem->DisplayDebugMessage(FinalMessage, FLinearColor::Green);
		}
	}
}
