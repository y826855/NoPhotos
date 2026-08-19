#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NPRelicHolderInterface.generated.h"

class AActor;

/** 사진 판정이 Grab 구현에 의존하지 않고 현재 소유 Relic을 조회하기 위한 계약입니다. */
UINTERFACE(BlueprintType)
class NOPHOTOS_API UNPRelicHolderInterface : public UInterface
{
	GENERATED_BODY()
};

class NOPHOTOS_API INPRelicHolderInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Relic")
	AActor* GetHeldRelic() const;
};
