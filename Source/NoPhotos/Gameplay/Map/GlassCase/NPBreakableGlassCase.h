#pragma once

#include "CoreMinimal.h"
#include "Chaos/ChaosNotifyHandlerInterface.h"
#include "GameFramework/Actor.h"
#include "Physics/Experimental/ChaosEventType.h"
#include "NPBreakableGlassCase.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;
class UGeometryCollectionComponent;
class APawn;

UCLASS(Blueprintable)
class NOPHOTOS_API ANPBreakableGlassCase : public AActor
{
	GENERATED_BODY()

public:
	ANPBreakableGlassCase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Glass Case")
	bool IsBroken() const { return bIsBroken; }

	UFUNCTION(BlueprintPure, Category = "Glass Case")
	bool IsUnlocked() const { return bIsUnlocked; }

	UFUNCTION(BlueprintPure, Category = "Glass Case")
	bool IsOpened() const { return bIsBroken || bIsUnlocked; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<UStaticMeshComponent> CaseMesh;

	/** 유리 부분의 충격만 감지하는 비가시성 고정 충돌 영역입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<UBoxComponent> GlassCollision;

	/** 열쇠의 충돌 컴포넌트가 이 영역에 겹치면 열쇠 조건을 검사합니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<UBoxComponent> LockVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Impact", meta = (ClampMin = "0.0"))
	float BreakImpactThreshold = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Destruction", meta = (ClampMin = "0.0"))
	float DestructionStrain = 100000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Destruction", meta = (ClampMin = "0.0", Units = "cm"))
	float StrainRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Destruction", meta = (ClampMin = "0"))
	int32 StrainPropagationDepth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Destruction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StrainPropagationFactor = 0.5f;

	/** 캐릭터가 유리 파편을 밀 때 파편에 더해지는 속도 변화량입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Shard Push", meta = (ClampMin = "0.0"))
	float ShardPushStrength = 200.0f;

	/** 접촉 지점 주변에서 함께 밀려나는 파편의 범위입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Shard Push", meta = (ClampMin = "1.0", Units = "cm"))
	float ShardPushRadius = 75.0f;

	/** 같은 캐릭터의 연속 충돌로 파편이 과도하게 날아가는 것을 막습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Shard Push", meta = (ClampMin = "0.0", Units = "s"))
	float ShardPushCooldown = 0.08f;

	/** 유물을 끌고 갈 때 파편을 좌우로 밀어내는 속도 변화량입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Shard Push", meta = (ClampMin = "0.0"))
	float RelicShardPushStrength = 600.0f;

	/** 유물 충돌이 연속으로 발생할 때 좌우 밀기를 다시 적용할 간격입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Shard Push", meta = (ClampMin = "0.0", Units = "s"))
	float RelicShardPushCooldown = 0.03f;

	/** 블루프린트에 추가된 첫 Geometry Collection Component입니다. */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Glass Case|Destruction")
	TObjectPtr<UGeometryCollectionComponent> GeometryCollectionComponent;

	/** 지정하면 이 클래스 또는 자식 클래스만 열쇠로 인정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Key")
	TSubclassOf<AActor> KeyActorClass;

	/** None이 아니면 이 Actor Tag를 가진 액터만 열쇠로 인정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Key")
	FName KeyActorTag = TEXT("GlassCaseKey");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Key")
	bool bConsumeKeyOnUnlock = true;

	/** 열리거나 깨진 뒤 CaseMesh 충돌을 제거할지 결정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Collision")
	bool bDisableCaseCollisionWhenOpened = true;

	UPROPERTY(ReplicatedUsing = OnRep_CaseState, VisibleInstanceOnly, BlueprintReadOnly, Category = "Glass Case|State")
	bool bIsBroken = false;

	UPROPERTY(ReplicatedUsing = OnRep_CaseState, VisibleInstanceOnly, BlueprintReadOnly, Category = "Glass Case|State")
	bool bIsUnlocked = false;

	UPROPERTY(Replicated)
	FVector_NetQuantize10 BreakLocation = FVector::ZeroVector;

private:
	UFUNCTION()
	void HandleCaseHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION()
	void HandleLockOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_CaseState();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBreakGlass(FVector_NetQuantize10 InBreakLocation);

	UFUNCTION()
	void HandleChaosBreak(const FChaosBreakEvent& BreakEvent);

	UFUNCTION()
	void HandleGlassPhysicsCollision(const FChaosPhysicsCollisionInfo& CollisionInfo);

	bool IsValidKey(const AActor* KeyActor) const;
	void BreakCase(float ImpactStrength, const FVector& ImpactLocation);
	void UnlockCase(AActor* KeyActor);
	void ApplyCaseState();
	void ApplyGlassDestruction();
	void ApplyDestructionStrain();

	bool bDestructionApplied = false;
	TMap<TWeakObjectPtr<AActor>, double> LastShardPushTimes;
};
