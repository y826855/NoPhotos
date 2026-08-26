#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPRelicCaseKeyVolume.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class ANPRelicCaseKey;
class UNPGimmickProgressComponent;

UCLASS(Blueprintable)
class NOPHOTOS_API ANPRelicCaseKeyVolume : public AActor
{
	GENERATED_BODY()

public:
	ANPRelicCaseKeyVolume();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Relic Case Key")
	void OnUnlockSucceeded();

	UFUNCTION(BlueprintImplementableEvent, Category = "Relic Case Key")
	void OnKeyOverlapStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Relic Case Key")
	void OnKeyOverlapEnded();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastUnlockSucceeded();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartProgress();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastReverseProgress();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Relic Case Key|Components")
	TObjectPtr<UBoxComponent> LockVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Relic Case Key|Components")
	TObjectPtr<UNPGimmickProgressComponent> GimmickProgressComponent;

	/** 이 볼륨이 해금할 액터들입니다. */
	UPROPERTY(
		EditInstanceOnly,
		BlueprintReadOnly,
		Category = "Relic Case Key",
		meta = (MustImplement = "/Script/NoPhotos.NPLockable"))
	TArray<TObjectPtr<AActor>> UnlockTargets;

	private:
	UFUNCTION()
	void HandleLockBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleLockEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	UFUNCTION()
	void HandleProgressCompleted();

	UFUNCTION()
	void HandleProgressLost();

	void RequestUnlock();
	void RequestLock();
	bool SetTargetActorsLocked(bool bLocked);

	UPROPERTY(Transient)
	TObjectPtr<ANPRelicCaseKey> ActiveKey;

#pragma region Debug
#if WITH_EDITOR
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;
	void DrawDebugConnections() const;
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Relic Case Key|Debug")
	bool bDrawDebugConnections = true;

	UPROPERTY(EditAnywhere, Category = "Relic Case Key|Debug")
	FColor DebugConnectionColor = FColor::Cyan;

	UPROPERTY(
		EditAnywhere,
		Category = "Relic Case Key|Debug",
		meta = (ClampMin = "0.0"))
	float DebugConnectionThickness = 2.0f;
#endif
#pragma endregion Debug
};
