#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Relic/NPBaseRelic.h"
#include "NPBreakableRelic.generated.h"

class UPrimitiveComponent;
class UStaticMesh;

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

	UFUNCTION()
	void OnRep_IsBroken();

	UFUNCTION()
	void HandleRelicHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Relic|Breakable")
	TObjectPtr<UStaticMesh> IntactMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Relic|Breakable")
	TObjectPtr<UStaticMesh> BrokenMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Relic|Breakable", meta = (ClampMin = "0.0"))
	float BreakImpactThreshold = 5000.0f;

	UPROPERTY(ReplicatedUsing = OnRep_IsBroken, VisibleInstanceOnly, BlueprintReadOnly, Category = "Relic|Breakable")
	bool bIsBroken = false;

private:
	void BreakRelic(float ImpactStrength);
	void ApplyBrokenState();
};
