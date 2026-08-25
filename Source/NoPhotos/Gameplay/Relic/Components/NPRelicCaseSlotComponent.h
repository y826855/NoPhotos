#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "NPRelicCaseSlotComponent.generated.h"

class ANPBaseRelic;
class FLifetimeProperty;

UCLASS(ClassGroup = (Relic), meta = (BlueprintSpawnableComponent))
class NOPHOTOS_API UNPRelicCaseSlotComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UNPRelicCaseSlotComponent();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Relic Case Slot")
	ANPBaseRelic* GetSpawnedRelic() const { return SpawnedRelic; }

	/** 슬롯 Transform에 유물을 생성합니다. 서버에서만 유효합니다. */
	ANPBaseRelic* SpawnRelic(bool bInitiallyReleased);

	/** 생성된 유물을 케이스 잠금에서 해제합니다. */
	void ReleaseRelic();

protected:
	UFUNCTION()
	void OnRep_SpawnedRelic();

	UFUNCTION()
	void OnRep_IsRelicReleased();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic Case Slot")
	TSubclassOf<ANPBaseRelic> RelicClass;

	UPROPERTY(ReplicatedUsing = OnRep_SpawnedRelic, VisibleInstanceOnly, BlueprintReadOnly, Category = "Relic Case Slot")
	TObjectPtr<ANPBaseRelic> SpawnedRelic;

	UPROPERTY(ReplicatedUsing = OnRep_IsRelicReleased, VisibleInstanceOnly, BlueprintReadOnly, Category = "Relic Case Slot")
	bool bIsRelicReleased = false;

private:
	void ApplyRelicState();
};
