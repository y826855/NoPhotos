#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPVent.generated.h"

class UArrowComponent;
class UBoxComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class APawn;

UCLASS(Blueprintable)
class NOPHOTOS_API ANPVent : public AActor
{
	GENERATED_BODY()

public:
	ANPVent();

	UFUNCTION(BlueprintPure, Category = "Vent")
	ANPVent* GetConnectedVent() const { return ConnectedVent; }

	UFUNCTION(BlueprintPure, Category = "Vent")
	FTransform GetExitTransform() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleVentOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vent")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vent")
	TObjectPtr<UStaticMeshComponent> VentMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vent")
	TObjectPtr<UBoxComponent> InteractionVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vent")
	TObjectPtr<UArrowComponent> ExitPoint;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Vent", meta = (DisplayName = "Connect"))
	TObjectPtr<ANPVent> ConnectedVent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vent", meta = (ClampMin = "0.0"))
	float ArrivalBlockTime = 0.5f;

private:
	bool IsTeleportBlockedFor(const APawn* Pawn) const;
	void BlockTeleportFor(APawn* Pawn);

	TMap<TWeakObjectPtr<APawn>, double> TeleportBlockedUntil;
};
