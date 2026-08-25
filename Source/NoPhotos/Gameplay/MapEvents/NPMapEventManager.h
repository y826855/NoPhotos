#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NPMapEventManager.generated.h"

class ANPMapEvent;
class ANPMapEventLocationCollector;
class ANPMapEventSpawnPoint;
class UNPMapEventCatalog;
class ULevelStreamingDynamic;
class UWorld;
enum class ENPMapEventType : uint8;

/** GameState에 부착되어 서버의 맵 이벤트 스케줄과 실행 상태를 관리합니다. */
UCLASS(Blueprintable, ClassGroup = (MapEvent), meta = (BlueprintSpawnableComponent))
class NOPHOTOS_API UNPMapEventManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPMapEventManagerComponent();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Map Event")
	void StartEventScheduling();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Map Event")
	void StopEventScheduling();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Map Event")
	bool TriggerRandomEvent(ENPMapEventType EventType);

	/** 로드된 컬렉터가 자신의 Point/Volume 목록을 이 매니저에 제공하도록 등록합니다. */
	void RegisterLocationCollector(ANPMapEventLocationCollector* Collector);
	void UnregisterLocationCollector(ANPMapEventLocationCollector* Collector);

	/** SpawnGroup에 속한 Point 중 하나를 가중치로 선택합니다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Map Event|Locations")
	ANPMapEventSpawnPoint* FindRandomSpawnPoint(FGameplayTag SpawnGroup) const;

	/** SpawnGroup에 속한 Volume 중 실제 생성 가능한 임의의 지면 Transform을 찾습니다. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Map Event|Locations")
	bool FindRandomSpawnTransform(
		FGameplayTag SpawnGroup,
		FVector RequiredHalfExtent,
		FTransform& OutTransform) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 이 GameState에서 사용할 이벤트 정의와 가중치 목록입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Event")
	TObjectPtr<UNPMapEventCatalog> EventCatalog;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Event")
	bool bStartAutomatically = true;

	/** 게임 시작 후 A 타입 이벤트가 발생할 수 있는 최소 시간(분)입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Event|Schedule|Type A", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "min"))
	float TypeAMinimumTimeMinutes = 2.0f;

	/** 게임 시작 후 A 타입 이벤트가 발생할 수 있는 최대 시간(분)입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Event|Schedule|Type A", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "min"))
	float TypeAMaximumTimeMinutes = 4.0f;

	/** 게임 시작 후 B 타입 이벤트가 발생할 수 있는 최소 시간(분)입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Event|Schedule|Type B", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "min"))
	float TypeBMinimumTimeMinutes = 7.0f;

	/** 게임 시작 후 B 타입 이벤트가 발생할 수 있는 최대 시간(분)입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Event|Schedule|Type B", meta = (ClampMin = "0.0", UIMin = "0.0", Units = "min"))
	float TypeBMaximumTimeMinutes = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map Event")
	bool bAllowConcurrentEvents = false;

private:
	bool HasServerAuthority() const;
	void CollectExistingLocationCollectors();
	void CreateEventInstances();
	bool RequestEventStart(ANPMapEvent* EventInstance);
	bool LoadEventLocationLevel(
		ANPMapEvent* EventInstance,
		const TSoftObjectPtr<UWorld>& LocationLevelInstance,
		const FTransform& LocationLevelTransform);

	UFUNCTION()
	void HandleLocationLevelShown();

	UFUNCTION()
	void HandleLocationLevelUnloaded();

	UFUNCTION()
	void HandleEventWithLocationLevelFinished(ANPMapEvent* MapEvent);

	void UnloadActiveLocationLevel();
	void ScheduleEvent(ENPMapEventType EventType, float MinimumTimeMinutes, float MaximumTimeMinutes);
	void HandleTypeAEventTimer();
	void HandleTypeBEventTimer();
	bool HasActiveEvent() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ANPMapEvent>> EventInstances;

	/** 이벤트 인스턴스가 실행될 때 함께 스트리밍할 위치 Point/Volume 레벨입니다. */
	UPROPERTY(Transient)
	TMap<TObjectPtr<ANPMapEvent>, TSoftObjectPtr<UWorld>> EventLocationLevels;

	/** Level Instance로 변환하면서 생긴 원점 오프셋을 복원할 이벤트별 Transform입니다. */
	UPROPERTY(Transient)
	TMap<TObjectPtr<ANPMapEvent>, FTransform> EventLocationLevelTransforms;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ANPMapEventLocationCollector>> LocationCollectors;

	/** 위치 레벨이 준비된 뒤 시작할 이벤트입니다. */
	UPROPERTY(Transient)
	TObjectPtr<ANPMapEvent> PendingEvent;

	/** 현재 이벤트를 위해 로드 중이거나 로드된 위치 레벨입니다. */
	UPROPERTY(Transient)
	TObjectPtr<ULevelStreamingDynamic> ActiveLocationLevel;

	bool bLocationLevelTransitionInProgress = false;
	int32 LocationLevelInstanceSerial = 0;

	FTimerHandle TypeAEventTimer;
	FTimerHandle TypeBEventTimer;
};
