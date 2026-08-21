#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Physics/Experimental/ChaosEventType.h"
#include "NPBreakableGlassCase.generated.h"

class AActor;
class ANPBaseRelic;
class UBoxComponent;
class UGeometryCollectionComponent;
class UGrabbableComponent;
class UNPImpactReceiveComponent;
class UPhysicsConstraintComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class NOPHOTOS_API ANPBreakableGlassCase : public AActor
{
	GENERATED_BODY()

public:
	ANPBreakableGlassCase();

	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Glass Case")
	bool IsBroken() const { return bIsBroken; }

	UFUNCTION(BlueprintPure, Category = "Glass Case")
	bool IsUnlocked() const { return bIsUnlocked; }

	UFUNCTION(BlueprintPure, Category = "Glass Case")
	bool IsOpened() const { return bIsBroken || bIsUnlocked; }

	UFUNCTION(BlueprintPure, Category = "Glass Case|Relic")
	ANPBaseRelic* GetContainedRelic() const
	{
		return ContainedRelics.IsValidIndex(0)
			? ContainedRelics[0].Get()
			: nullptr;
	}

	UFUNCTION(BlueprintPure, Category = "Glass Case|Relic")
	TArray<ANPBaseRelic*> GetContainedRelics() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_IsBroken();

	UFUNCTION()
	void OnRep_IsUnlocked();

	UFUNCTION()
	void OnRep_ContainedRelics();

	UFUNCTION()
	void HandleFullyDecayed();

	/** 논리적인 파괴 상태가 최초 적용될 때 한 번 호출됩니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Glass Case|Breakable")
	void OnGlassCaseBroken();

	/** 충돌 피해가 적용될 때 호출됩니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Glass Case|Breakable")
	void OnGlassCaseDamaged(int32 Damage, int32 CurrentHealth, int32 MaxHealth);

	/** 열쇠로 정상 해제됐을 때 한 번 호출됩니다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Glass Case|Key")
	void OnGlassCaseUnlocked();

	/** 서버에서 확정된 파괴를 현재 접속 중인 모든 클라이언트에 전달합니다. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastBreakGlassCase(FVector_NetQuantize10 InBreakLocation);

	/** 본체, 뚜껑, 자물쇠와 SpawnPoint의 크기를 서로 분리하는 루트입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** 표시, 충돌, 물리, 파괴를 담당하는 케이스 본체입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<UGeometryCollectionComponent> GlassGeometryCollection;

	/** NPBreakableRelic과 동일한 충돌 내구도 처리 컴포넌트입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<UNPImpactReceiveComponent> ImpactReceiveComponent;

	/** 열쇠 해제 후 플레이어가 잡아 움직일 뚜껑입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<UStaticMeshComponent> LidMesh;

	/** LidMesh를 케이스 본체에 고정하는 힌지입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<UPhysicsConstraintComponent> LidHingeConstraint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<UGrabbableComponent> LidGrabbableComponent;

	/** 열쇠의 충돌 컴포넌트가 이 영역에 겹치면 열쇠 조건을 검사합니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<UBoxComponent> LockVolume;

	/** 런타임에 생성할 유물의 위치와 회전 기준입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<USceneComponent> RelicSpawnPoint;

	/** 임시 설정: 이 케이스 안에 생성할 유물 Blueprint 클래스를 직접 지정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Relic")
	TSubclassOf<ANPBaseRelic> RelicBlueprintClass;

	/** RelicSpawnPoint에 생성되는 유물의 최종 크기입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Relic", meta = (ClampMin = "0.01"))
	FVector SpawnedRelicScale = FVector::OneVector;

	/** RelicSpawnPoint에서 생성할 유물 수입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Relic", meta = (ClampMin = "0", UIMin = "0"))
	int32 RelicSpawnCount = 1;

	/** 여러 유물을 생성할 때 RelicSpawnPoint 로컬 Y축 방향의 간격입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Relic", meta = (ClampMin = "0.0", Units = "cm"))
	float RelicSpawnSpacing = 50.0f;

	/** 지정하면 이 클래스 또는 자식 클래스만 열쇠로 인정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Key")
	TSubclassOf<AActor> KeyActorClass;

	/** None이 아니면 이 Actor Tag를 가진 액터만 열쇠로 인정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Key")
	FName KeyActorTag = TEXT("GlassCaseKey");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Key")
	bool bConsumeKeyOnUnlock = true;

	/** 닫힌 상태를 기준으로 힌지가 움직일 수 있는 최대 각도입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Lid", meta = (ClampMin = "1.0", ClampMax = "175.0", Units = "deg"))
	float LidOpenAngle = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Lid", meta = (ClampMin = "0.0"))
	float LidAngularDamping = 5.0f;

	UPROPERTY(ReplicatedUsing = OnRep_IsBroken, VisibleInstanceOnly, BlueprintReadOnly, Category = "Glass Case|State")
	bool bIsBroken = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsUnlocked, VisibleInstanceOnly, BlueprintReadOnly, Category = "Glass Case|State")
	bool bIsUnlocked = false;

	UPROPERTY(Replicated)
	FVector_NetQuantize10 BreakLocation = FVector::ZeroVector;

	/** 서버가 생성한 케이스 내부 유물들입니다. */
	UPROPERTY(ReplicatedUsing = OnRep_ContainedRelics, VisibleInstanceOnly, BlueprintReadOnly, Category = "Glass Case|Relic")
	TArray<TObjectPtr<ANPBaseRelic>> ContainedRelics;

private:
	UFUNCTION()
	void HandleGeometryBreak(const FChaosBreakEvent& BreakEvent);

	UFUNCTION()
	void HandleLockOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void HandleDurabilityDamaged(
		int32 Damage,
		int32 CurrentHealth,
		int32 MaxHealth);
	void HandleDurabilityDepleted(const FVector& ImpactLocation);
	bool IsValidKey(const AActor* KeyActor) const;
	void BreakGlassCase(const FVector& ImpactLocation);
	void UnlockCase(AActor* KeyActor);
	void ApplyCaseState();
	void ApplyBrokenState();
	void ConfigureLidHinge();
	void BreakRootCluster();
	void SpawnContainedRelics();
	void UnlockContainedRelic();
	void UpdateContainedRelicCollision();

	bool bBrokenEventDispatched = false;
	bool bUnlockedEventDispatched = false;
	bool bClusterBreakApplied = false;
	bool bActualBreakLogged = false;
};
