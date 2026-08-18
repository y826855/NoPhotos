#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPRelicGimmickComponent.generated.h"

class FLifetimeProperty;

DECLARE_MULTICAST_DELEGATE(FOnRelicGimmickCompleted);

UCLASS(Abstract, Blueprintable, ClassGroup=(Relic))
class NOPHOTOS_API UNPRelicGimmickComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPRelicGimmickComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Relic Gimmick")
	bool IsCompleted() const { return bIsCompleted; }

	UFUNCTION(BlueprintCallable, Category="Relic Gimmick")
	void CompleteGimmick();

	FOnRelicGimmickCompleted OnCompleted;

private:
	UFUNCTION()
	void OnRep_IsCompleted();

	UPROPERTY(ReplicatedUsing=OnRep_IsCompleted, VisibleInstanceOnly, BlueprintReadOnly, Category="Relic Gimmick", meta=(AllowPrivateAccess="true"))
	bool bIsCompleted = false;
};
