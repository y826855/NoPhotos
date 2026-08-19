#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPBreakableGlassCase.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

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

	/** 열쇠의 충돌 컴포넌트가 이 영역에 겹치면 열쇠 조건을 검사합니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Glass Case|Components")
	TObjectPtr<UBoxComponent> LockVolume;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glass Case|Visual")
	TObjectPtr<UStaticMesh> IntactMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glass Case|Visual")
	TObjectPtr<UStaticMesh> BrokenMesh;

	/** 열쇠로 정상 해제됐을 때 사용할 메시입니다. 비어 있으면 IntactMesh를 유지합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Glass Case|Visual")
	TObjectPtr<UStaticMesh> UnlockedMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Glass Case|Impact", meta = (ClampMin = "0.0"))
	float BreakImpactThreshold = 5000.0f;

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

	bool IsValidKey(const AActor* KeyActor) const;
	void BreakCase(float ImpactStrength);
	void UnlockCase(AActor* KeyActor);
	void ApplyCaseState();
};
