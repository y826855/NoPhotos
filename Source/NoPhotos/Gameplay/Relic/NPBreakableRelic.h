#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Relic/NPBaseRelic.h"
#include "Physics/Experimental/ChaosEventType.h"
#include "NPBreakableRelic.generated.h"

class UPrimitiveComponent;
class UGeometryCollectionComponent;
class UBoxComponent;

UCLASS(Blueprintable)
class NOPHOTOS_API ANPBreakableRelic : public ANPBaseRelic
{
	GENERATED_BODY()

public:
	ANPBreakableRelic();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Relic|Breakable")
	bool IsBroken() const { return bIsBroken; }

protected:
	virtual void BeginPlay() override;

	/** 파괴 전 잡기, 중력, 충격 감지를 담당하는 비가시성 단순 충돌 프록시입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionProxy;

	UFUNCTION()
	void OnRep_IsBroken();

	/** 서버에서 확정된 파괴를 현재 접속 중인 모든 클라이언트에 전달합니다. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastBreakRelic(FVector_NetQuantize10 InBreakLocation);

	UFUNCTION()
	void HandleRelicHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION()
	void HandleChaosBreak(const FChaosBreakEvent& BreakEvent);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Breakable", meta = (ClampMin = "0.0"))
	float BreakImpactThreshold = 5000.0f;

	/** 잡기 Constraint가 생성될 때 발생하는 순간적인 보정 충격을 파괴 판정에서 제외하는 시간입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Breakable", meta = (ClampMin = "0.0", Units = "s"))
	float GrabImpactIgnoreDuration = 0.25f;

	/** 파손 조건을 만족했을 때 Geometry Collection에 적용할 외부 Strain입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Destruction", meta = (ClampMin = "0.0"))
	float DestructionStrain = 100000.0f;

	/** 충돌 지점을 중심으로 Strain을 적용할 반경입니다. 0이면 맞은 클러스터에만 적용합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Destruction", meta = (ClampMin = "0.0", Units = "cm"))
	float StrainRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Destruction", meta = (ClampMin = "0"))
	int32 StrainPropagationDepth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Destruction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StrainPropagationFactor = 0.5f;

	/** 블루프린트에 추가된 첫 번째 Geometry Collection Component를 런타임에 보관합니다. */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "Relic|Destruction")
	TObjectPtr<UGeometryCollectionComponent> GeometryCollectionComponent;

	UPROPERTY(ReplicatedUsing = OnRep_IsBroken, VisibleInstanceOnly, BlueprintReadOnly, Category = "Relic|Breakable")
	bool bIsBroken = false;

	UPROPERTY(Replicated)
	FVector_NetQuantize10 BreakLocation = FVector::ZeroVector;

private:
	void BreakRelic(float ImpactStrength, const FVector& ImpactLocation);
	void HandleBreakableGrabStarted(UPrimitiveComponent* GrabbedComponent);
	void ApplyBrokenState();
	void ApplyDestructionStrain();
	void ReportDestructionResult();
	void ShowBrokenDebugMessage() const;
	bool bStrainApplied = false;
	int32 ChaosBreakEventCount = 0;
	double IgnoreImpactUntilTime = 0.0;
};
