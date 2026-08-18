#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "NPBaseRelic.generated.h"

class UGrabbableComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class FLifetimeProperty;

UCLASS(Abstract, Blueprintable)
class NOPHOTOS_API ANPBaseRelic : public AActor
{
	GENERATED_BODY()

public:
	ANPBaseRelic();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Relic")
	bool IsDisplayed() const { return bIsDisplayed; }

	UFUNCTION(BlueprintPure, Category="Relic")
	bool IsUnlocked() const { return bIsUnlocked; }

	void SetUnlocked(bool bUnlocked);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_IsDisplayed();

	void ReleaseFromDisplay();
	void HandleGrabStarted(UPrimitiveComponent* GrabbedComponent);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> RelicMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UGrabbableComponent> GrabbableComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Relic")
	FDataTableRowHandle RelicData;

	UPROPERTY(ReplicatedUsing=OnRep_IsDisplayed, VisibleInstanceOnly, BlueprintReadOnly, Category="Relic")
	bool bIsDisplayed = true;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Relic")
	bool bIsUnlocked = true;
};
