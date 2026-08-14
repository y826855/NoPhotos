#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNPRoom, Log, All);

namespace NPRoomLog
{
	NOPHOTOS_API void Info(const UObject* WorldContextObject, const FString& Message);
	NOPHOTOS_API void Warning(const UObject* WorldContextObject, const FString& Message);
}
