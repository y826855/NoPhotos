#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NPLockable.generated.h"

UINTERFACE(BlueprintType)
class NOPHOTOS_API UNPLockable : public UInterface
{
	GENERATED_BODY()
};

class NOPHOTOS_API INPLockable
{
	GENERATED_BODY()

public:
	/** 잠금 상태 변경을 요청하고, 실제 상태가 변경됐으면 true를 반환합니다. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lockable")
	bool TrySetLocked(bool bLocked);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lockable")
	bool IsLocked() const;
};
