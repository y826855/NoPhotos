#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPRelicCaseKey.generated.h"

class UGrabbableComponent;
class UBoxComponent;

UCLASS(Blueprintable)
class NOPHOTOS_API ANPRelicCaseKey : public AActor
{
	GENERATED_BODY()

public:
	ANPRelicCaseKey();

	/** 키가 해금에 사용됐을 때 서버에서 호출합니다. */
	void NotifyUnlockSucceeded();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Relic Case Key")
	void OnUnlockSucceeded();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastUnlockSucceeded();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Relic Case Key|Components")
	TObjectPtr<UBoxComponent> KeyCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Relic Case Key|Components")
	TObjectPtr<UGrabbableComponent> GrabbableComponent;
};
