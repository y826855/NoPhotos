#pragma once

#include "CoreMinimal.h"
#include "Room/NPRoomGameMode.h"
#include "NPGameMode.generated.h"

class APlayerState;
class ANPPlayerController;
class UWorld;

UCLASS()
class NOPHOTOS_API ANPGameMode : public ANPRoomGameMode
{
	GENERATED_BODY()

public:
	ANPGameMode();
};
