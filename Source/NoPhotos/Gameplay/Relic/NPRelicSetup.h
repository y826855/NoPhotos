#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPRelicSetup.generated.h"

class ANPBaseRelic;
class UNPRelicGimmickComponent;

UCLASS(Blueprintable)
class NOPHOTOS_API ANPRelicSetup : public AActor
{
	GENERATED_BODY()

public:
	ANPRelicSetup();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Relic Setup")
	TObjectPtr<ANPBaseRelic> Relic;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Relic Setup")
	TArray<TObjectPtr<AActor>> GimmickActors;

private:
	void CollectGimmicks();
	void CollectGimmicksFromActor(AActor* GimmickActor);
	void HandleGimmickCompleted();
	void RefreshRelicLock();

	UPROPERTY(Transient, VisibleInstanceOnly, Category="Relic Setup|Debug")
	TArray<TObjectPtr<UNPRelicGimmickComponent>> Gimmicks;
};
