#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "NPMapEventSpawnPoint.generated.h"

class UArrowComponent;

/** 디자이너가 정확한 위치와 방향을 지정하는 맵 이벤트용 생성 후보 지점입니다. */
UCLASS(Blueprintable)
class NOPHOTOS_API ANPMapEventSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ANPMapEventSpawnPoint();

	UFUNCTION(BlueprintPure, Category = "Map Event|Spawn Point")
	bool SupportsSpawnGroup(FGameplayTag SpawnGroup) const;

	UFUNCTION(BlueprintPure, Category = "Map Event|Spawn Point")
	bool IsSpawnEnabled() const { return bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Map Event|Spawn Point")
	float GetSelectionWeight() const { return SelectionWeight; }

	/** true이면 자신이 속한 Collector의 기본 SpawnGroup도 함께 사용합니다. */
	UFUNCTION(BlueprintPure, Category = "Map Event|Spawn Point")
	bool UsesCollectorSpawnGroups() const { return bUseCollectorSpawnGroups; }

protected:
	/** 위치와 정면 방향을 에디터에서 확인하기 위한 마커입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> LocationMarker;

	/** 이 Point를 사용할 수 있는 이벤트 생성 그룹입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Event|Spawn Point")
	FGameplayTagContainer SupportedSpawnGroups;

	/** Level Instance Collector의 기본 그룹을 상속합니다. 개별 태그만 쓰려면 끕니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Event|Spawn Point")
	bool bUseCollectorSpawnGroups = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Event|Spawn Point")
	bool bEnabled = true;

	/** 같은 SpawnGroup의 Point가 여러 개일 때 사용할 상대 가중치입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map Event|Spawn Point", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SelectionWeight = 1.0f;
};
