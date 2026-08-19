#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNPMainGame, Log, All);

namespace NPMainGameLog
{
	NOPHOTOS_API void Info(const UObject* WorldContextObject, const FString& Message);
}
