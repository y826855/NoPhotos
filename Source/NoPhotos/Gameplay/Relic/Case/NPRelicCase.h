#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPRelicCase.generated.h"

class ANPBaseRelic;
class FLifetimeProperty;
class UGeometryCollectionComponent;
class UNPImpactReceiveComponent;
class USceneComponent;

UCLASS(Blueprintable)
class NOPHOTOS_API ANPRelicCase : public AActor
{
	GENERATED_BODY()

public:
	ANPRelicCase();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Relic Case")
	bool IsBroken() const { return bIsBroken; }

	UFUNCTION(BlueprintPure, Category = "Relic Case")
	bool IsUnlocked() const { return bIsUnlocked; }

	UFUNCTION(BlueprintPure, Category = "Relic Case")
	bool IsAccessible() const { return bIsBroken || bIsUnlocked; }

	UFUNCTION(BlueprintPure, Category = "Relic Case|Relic")
	ANPBaseRelic* GetContainedRelic() const { return ContainedRelic; }

	/** 외부 기믹이 서버에서 케이스를 해금할 때 호출합니다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Relic Case")
	bool UnlockCase();

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_IsBroken();

	UFUNCTION()
	void OnRep_IsUnlocked();

	UFUNCTION()
	void OnRep_ContainedRelic();

	UFUNCTION(BlueprintImplementableEvent, Category = "Relic Case")
	void OnCaseBroken();

	UFUNCTION(BlueprintImplementableEvent, Category = "Relic Case")
	void OnCaseUnlocked();

	UFUNCTION(BlueprintImplementableEvent, Category = "Relic Case")
	void OnCaseDamaged(float RemainingHealthRatio);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBreakCase(FVector_NetQuantize10 InBreakLocation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Relic Case|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** 파괴할 Geometry Collection들을 배치하는 기준 컴포넌트입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Relic Case|Components")
	TObjectPtr<USceneComponent> GeometryScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Relic Case|Components")
	TObjectPtr<UNPImpactReceiveComponent> ImpactReceiveComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Relic Case|Components")
	TObjectPtr<USceneComponent> RelicSpawnPoint;

	/** GeometryScene 아래에서 수집된 Geometry Collection들입니다. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Relic Case|Components")
	TArray<TObjectPtr<UGeometryCollectionComponent>> CaseGeometryCollections;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic Case|Relic")
	TSubclassOf<ANPBaseRelic> RelicBlueprintClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic Case|Relic", meta = (ClampMin = "0.01"))
	FVector SpawnedRelicScale = FVector::OneVector;

	UPROPERTY(ReplicatedUsing = OnRep_IsBroken, VisibleInstanceOnly, BlueprintReadOnly, Category = "Relic Case|State")
	bool bIsBroken = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsUnlocked, VisibleInstanceOnly, BlueprintReadOnly, Category = "Relic Case|State")
	bool bIsUnlocked = false;

	UPROPERTY(Replicated)
	FVector_NetQuantize10 BreakLocation = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing = OnRep_ContainedRelic, VisibleInstanceOnly, BlueprintReadOnly, Category = "Relic Case|Relic")
	TObjectPtr<ANPBaseRelic> ContainedRelic;

private:
	void CollectCaseGeometryCollections();
	void InitializeGeometryCollections();
	void HandleDurabilityDamaged(
		int32 Damage,
		int32 CurrentHealth,
		int32 MaxHealth);
	void HandleDurabilityDepleted(const FVector& ImpactLocation);
	void BreakCase(const FVector& ImpactLocation);
	void ApplyCaseState();
	void ApplyBrokenState();
	void SpawnContainedRelic();
	void UnlockContainedRelic();
	void UpdateContainedRelicCollision();

	bool bBrokenStateApplied = false;
	bool bBrokenEventDispatched = false;
	bool bUnlockedEventDispatched = false;
};
