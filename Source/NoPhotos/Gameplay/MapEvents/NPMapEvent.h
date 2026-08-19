#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPMapEvent.generated.h"

UCLASS(Abstract, Blueprintable)
class NOPHOTOS_API ANPMapEvent : public AActor
{
	GENERATED_BODY()

public:
	ANPMapEvent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Map Event")
	bool StartEvent();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Map Event")
	void FinishEvent();

	UFUNCTION(BlueprintPure, Category = "Map Event")
	bool IsEventActive() const { return bIsActive; }

	bool CanStartEvent(double ServerTimeSeconds) const;
	float GetSelectionWeight() const { return SelectionWeight; }

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Map Event")
	void ApplyEventState(bool bNewActive);
	virtual void ApplyEventState_Implementation(bool bNewActive);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Event", meta = (ClampMin = "0.0"))
	float Duration = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Event", meta = (ClampMin = "0.0"))
	float Cooldown = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Event", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

private:
	UFUNCTION()
	void OnRep_IsActive();

	UPROPERTY(ReplicatedUsing = OnRep_IsActive)
	bool bIsActive = false;

	double LastFinishedServerTime = -1.0;
	FTimerHandle DurationTimer;
};
