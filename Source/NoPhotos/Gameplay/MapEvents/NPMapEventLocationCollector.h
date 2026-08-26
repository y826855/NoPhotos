#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "NPMapEventTypes.h"
#include "NPMapEventLocationCollector.generated.h"

class ANPMapEventSpawnPoint;
class ANPMapEventSpawnVolume;

/**
 * 현재 맵에 배치된 이벤트 SpawnPoint와 SpawnVolume을 한 번 수집해 제공하는 레벨 헬퍼입니다.
 * 실제 이벤트 실행과 스케줄은 담당하지 않습니다.
 */
UCLASS(Blueprintable)
class NOPHOTOS_API ANPMapEventLocationCollector : public AActor
{
	GENERATED_BODY()

public:
	ANPMapEventLocationCollector();

	UFUNCTION(BlueprintPure, Category = "Map Event|Locations")
	bool SupportsSpawnGroup(FGameplayTag SpawnGroup) const;

	/** 현재 로드된 월드에서 선택한 종류의 위치 목록만 다시 수집합니다. */
	UFUNCTION(BlueprintCallable, Category = "Map Event|Locations")
	void RefreshLocations(ENPMapEventLocationSource LocationSource = ENPMapEventLocationSource::Both);

	UFUNCTION(BlueprintCallable, Category = "Map Event|Locations")
	void GetSpawnPointsForGroup(
		FGameplayTag SpawnGroup,
		TArray<ANPMapEventSpawnPoint*>& OutSpawnPoints) const;

	UFUNCTION(BlueprintCallable, Category = "Map Event|Locations")
	void GetSpawnVolumesForGroup(
		FGameplayTag SpawnGroup,
		TArray<ANPMapEventSpawnVolume*>& OutSpawnVolumes) const;

	/** 같은 그룹의 사용 가능한 Point 중 가중치에 따라 하나를 반환합니다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Map Event|Locations")
	ANPMapEventSpawnPoint* FindRandomSpawnPoint(FGameplayTag SpawnGroup) const;

	/** 같은 그룹의 Volume을 가중치로 선택하고 실제 생성 가능한 지면 Transform을 찾습니다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Map Event|Locations")
	bool FindRandomSpawnTransformInVolume(
		FGameplayTag SpawnGroup,
		FVector RequiredHalfExtent,
		FTransform& OutTransform) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 이 Collector와 같은 Level Instance 안의 Point/Volume이 기본으로 상속할 그룹입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Event|Locations")
	FGameplayTagContainer DefaultSpawnGroups;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Map Event|Locations")
	TArray<TObjectPtr<ANPMapEventSpawnPoint>> CollectedSpawnPoints;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Map Event|Locations")
	TArray<TObjectPtr<ANPMapEventSpawnVolume>> CollectedSpawnVolumes;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Map Event|Locations")
	ENPMapEventLocationSource LastCollectionSource = ENPMapEventLocationSource::Both;
};
