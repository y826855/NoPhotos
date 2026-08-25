#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPMapEventTypes.h"
#include "NPMapEvent.generated.h"

class ANPMapEvent;
class UNPMapEventDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FNPMapEventLifecycleSignature,
	ANPMapEvent*,
	MapEvent);

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

	/** 카탈로그가 이벤트 액터를 생성할 때 정의와 해당 목록의 가중치를 주입합니다. */
	void InitializeEvent(UNPMapEventDefinition* InEventDefinition, float InSelectionWeight);

	UFUNCTION(BlueprintPure, Category = "Map Event|Metadata")
	UNPMapEventDefinition* GetEventDefinition() const { return EventDefinition; }

	UFUNCTION(BlueprintPure, Category = "Map Event|Metadata")
	FName GetEventId() const;

	UFUNCTION(BlueprintPure, Category = "Map Event|Metadata")
	FText GetEventDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Map Event|Metadata")
	ENPMapEventType GetEventType() const;

	UFUNCTION(BlueprintPure, Category = "Map Event|Metadata")
	ENPMapEventScale GetEventScale() const;

	UFUNCTION(BlueprintPure, Category = "Map Event|Metadata")
	float GetEventDuration() const;

	/** 이 이벤트가 지정된 A/B 발생 시간의 후보로 들어갈 수 있는지 반환합니다. */
	UFUNCTION(BlueprintPure, Category = "Map Event|Metadata")
	bool CanRunAtEventTime(ENPMapEventType EventTimeType) const;

	/** 초대형 이벤트는 일반 시간표와 분리해 단독으로 실행해야 합니다. */
	UFUNCTION(BlueprintPure, Category = "Map Event|Metadata")
	bool RequiresStandaloneExecution() const { return GetEventScale() == ENPMapEventScale::Colossal; }

	bool CanStartEvent() const;

	UFUNCTION(BlueprintPure, Category = "Map Event|Selection")
	float GetSelectionWeight() const { return RuntimeSelectionWeight; }

	/** 서버와 클라이언트 각각에서 이벤트 상태가 적용된 직후 호출됩니다. */
	UPROPERTY(BlueprintAssignable, Category = "Map Event|Lifecycle")
	FNPMapEventLifecycleSignature OnEventStarted;

	/** 서버와 클라이언트 각각에서 이벤트 상태가 해제된 직후 호출됩니다. */
	UPROPERTY(BlueprintAssignable, Category = "Map Event|Lifecycle")
	FNPMapEventLifecycleSignature OnEventFinished;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Map Event")
	void ApplyEventState(bool bNewActive);
	virtual void ApplyEventState_Implementation(bool bNewActive);

	/** 새 이벤트 정의 에셋으로 이전하기 전 네이티브 이벤트를 위한 임시 호환값입니다. */
	UPROPERTY()
	FName EventId = NAME_None;

	UPROPERTY()
	FText EventDisplayName;

	UPROPERTY()
	ENPMapEventType EventType = ENPMapEventType::TypeA;

	UPROPERTY()
	ENPMapEventScale EventScale = ENPMapEventScale::Small;

	UPROPERTY()
	float Duration = 8.0f;

private:
	UFUNCTION()
	void OnRep_IsActive();

	/** 서버가 선택한 정의이며 초기 액터 복제를 통해 클라이언트에도 전달됩니다. */
	UPROPERTY(Replicated)
	TObjectPtr<UNPMapEventDefinition> EventDefinition;

	UPROPERTY(ReplicatedUsing = OnRep_IsActive)
	bool bIsActive = false;

	/** 카탈로그 엔트리에서 주입된 서버 런타임 선택 가중치입니다. */
	float RuntimeSelectionWeight = 1.0f;

	FTimerHandle DurationTimer;
};
