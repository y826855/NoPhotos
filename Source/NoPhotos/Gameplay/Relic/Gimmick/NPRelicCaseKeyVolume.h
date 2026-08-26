#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPRelicCaseKeyVolume.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

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

	UFUNCTION(NetMulticast, Reliable)
	void MulticastUnlockSucceeded();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Relic Case Key|Components")
	TObjectPtr<UBoxComponent> LockVolume;

	/** 해금 성공 시 사용한 열쇠를 제거합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic Case Key")
	bool bConsumeKeyOnUnlock = true;

	/** 이 볼륨이 해금할 액터들입니다. */
	UPROPERTY(
		EditInstanceOnly,
		BlueprintReadOnly,
		Category = "Relic Case Key",
		meta = (MustImplement = "/Script/NoPhotos.NPLockableInterface"))
	TArray<TObjectPtr<AActor>> UnlockTargets;

private:
	UFUNCTION()
	void HandleLockOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	bool UnlockTargetActors();

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
