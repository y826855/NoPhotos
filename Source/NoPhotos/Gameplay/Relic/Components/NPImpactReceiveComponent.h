#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPImpactReceiveComponent.generated.h"

class UPrimitiveComponent;
class AActor;

DECLARE_MULTICAST_DELEGATE_OneParam(
	FOnDurabilityDepleted,
	const FVector&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(
	FOnDurabilityDamaged,
	int32,
	int32,
	int32);

UCLASS(ClassGroup=(Relic), meta=(BlueprintSpawnableComponent))
class NOPHOTOS_API UNPImpactReceiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPImpactReceiveComponent();

	void IgnoreGrabImpact();

	FOnDurabilityDamaged OnDamaged;
	FOnDurabilityDepleted OnDepleted;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Durability", meta=(ClampMin="0.0"))
	float MinImpactThreshold = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Durability", meta=(ClampMin="0.0"))
	float MaxImpactThreshold = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Durability", meta=(ClampMin="1"))
	int32 MinDamage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Durability", meta=(ClampMin="1"))
	int32 MaxDamage = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Durability", meta=(ClampMin="1"))
	int32 MaxHealth = 10;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Durability")
	int32 CurrentHealth = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Durability", meta=(ClampMin="0.0", Units="s"))
	float DamageCooldown = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Durability", meta=(ClampMin="0.0", Units="s"))
	float GrabImpactIgnoreDuration = 0.25f;

private:
	double IgnoreDamageUntilTime = 0.0;
	double NextDamageAllowedTime = 0.0;
};
