#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NPRoomSubsystem.generated.h"

class APlayerController;
class UCanvas;
class UNetDriver;
class UWorld;

//UI
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFindRoomsCompleteDelegate, const TArray<int32>&, RoomIndices);

struct FNPRoomDebugMessage
{
	FString Message;
	FLinearColor Color = FLinearColor::White;
	double ExpirationTime = 0.0;
};

enum class ENPRoomExitAction : uint8
{
	None,
	BecomeHost,
	RejoinMigratedRoom
};

UCLASS()
class NOPHOTOS_API UNPRoomSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Room")
	bool HostRoom();

	UFUNCTION(BlueprintCallable, Category = "Room")
	bool FindRooms();

	UFUNCTION(BlueprintCallable, Category = "Room")
	bool JoinRoom(int32 RoomNumber);

	void LeaveRoom();
	void BeginHostMigration(const FString& MigrationId, bool bBecomeHost);

	void UpdateRoomPlayerCount(int32 PlayerCount);
	void MarkRoomInGame();

	void LogOnlineServiceStatus();
	void DisplayDebugMessage(const FString& Message, const FLinearColor& Color);

private:
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void BeginExit(ENPRoomExitAction ExitAction, const FString& MigrationId);
	void TravelToStandaloneMenu();
	void RetryMigrationSearch();
	void ScheduleMigrationSearchRetry();
	void DrawDebugMessages(UCanvas* Canvas, APlayerController* PlayerController);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	TArray<int32> ListedRoomResultIndices;
	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle DebugDrawHandle;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle PostLoadMapHandle;
	FTimerHandle MigrationSearchTimer;
	ENPRoomExitAction PendingExitAction = ENPRoomExitAction::None;
	FString PendingMigrationId;
	FString ReturnMapPath;
	int32 MigrationSearchAttempts = 0;
	bool bSearchingForMigration = false;
	bool bHasLoggedOnlineServiceStatus = false;
	TArray<FNPRoomDebugMessage> DebugMessages;
	
#pragma region UI
public:
	UPROPERTY(BlueprintAssignable, Category = "Room")
	FOnFindRoomsCompleteDelegate OnFindRoomsComplete;

	// ListedRoomResultIndices를 외부(UI)에서 참조할 수 있도록 Getter 추가
	const TArray<int32>& GetListedRoomResultIndices() const { return ListedRoomResultIndices; }
#pragma endregion
};
