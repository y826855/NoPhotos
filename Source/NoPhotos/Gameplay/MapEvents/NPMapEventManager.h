#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPMapEventManager.generated.h"

class ANPMapEvent;

UCLASS(Blueprintable)
class NOPHOTOS_API ANPMapEventManager : public AActor
{
	GENERATED_BODY()

public:
	ANPMapEventManager();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Map Event")
	void StartEventScheduling();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Map Event")
	void StopEventScheduling();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Map Event")
	bool TriggerRandomEvent();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Event")
	TArray<TSubclassOf<ANPMapEvent>> EventClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Event")
	bool bStartAutomatically = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Event", meta = (ClampMin = "0.0"))
	float InitialDelay = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Event", meta = (ClampMin = "0.1"))
	float MinimumInterval = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Event", meta = (ClampMin = "0.1"))
	float MaximumInterval = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Event")
	bool bAllowConcurrentEvents = false;

private:
	void CreateEventInstances();
	void ScheduleNextEvent(float Delay);
	void HandleEventTimer();
	bool HasActiveEvent() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ANPMapEvent>> EventInstances;

	FTimerHandle EventTimer;
};
