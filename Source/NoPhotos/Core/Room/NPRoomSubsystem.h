#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NPRoomSubsystem.generated.h"

class APlayerController;
class UCanvas;
class UNetDriver;
class UWorld;

struct FNPRoomDebugMessage
{
	FString Message;
	FLinearColor Color = FLinearColor::White;
	double ExpirationTime = 0.0;
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
	bool JoinLocalRoom();

	void DisplayDebugMessage(const FString& Message, const FLinearColor& Color);

private:
	void DrawDebugMessages(UCanvas* Canvas, APlayerController* PlayerController);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

	FDelegateHandle DebugDrawHandle;
	FDelegateHandle NetworkFailureHandle;
	TArray<FNPRoomDebugMessage> DebugMessages;
};
