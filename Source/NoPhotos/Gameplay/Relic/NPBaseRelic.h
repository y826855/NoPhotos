#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "NPBaseRelic.generated.h"

class UGrabbableComponent;
class UPrimitiveComponent;
class FLifetimeProperty;
class APlayerState;

UCLASS(Abstract, Blueprintable)
class NOPHOTOS_API ANPBaseRelic : public AActor
{
	GENERATED_BODY()

public:
	ANPBaseRelic(
		const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Relic")
	bool IsDisplayed() const { return bIsDisplayed; }

	UFUNCTION(BlueprintPure, Category="Relic")
	bool IsUnlocked() const { return bIsUnlocked; }

	UFUNCTION(BlueprintPure, Category="Relic|Delivery")
	bool IsReturned() const { return bIsReturned; }

	/** 서버에서만 유효한 반환 점수 지급 대상입니다. */
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category="Relic|Delivery")
	APlayerState* GetLastCarrierPlayerState() const { return LastCarrierPlayerState; }

	/** 서버에서만 유효한 고유 촬영자 수입니다. */
	UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category="Relic|Delivery")
	int32 GetEvidencePhotographerCount() const { return EvidencePhotographers.Num(); }

	UFUNCTION(BlueprintPure, Category="Relic|Delivery")
	int32 GetBasePrice() const;

	void SetUnlocked(bool bUnlocked);
	void SetLastCarrierPlayerState(APlayerState* PlayerState);
	bool RegisterEvidencePhotographer(APlayerState* Photographer);
	bool TryMarkReturned();

protected:
	static const FName RelicComponentName;

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_IsDisplayed();

	UFUNCTION()
	void OnRep_IsReturned();

	void ReleaseFromDisplay();
	void HandleGrabStarted(UPrimitiveComponent* GrabbedComponent);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPrimitiveComponent> RelicMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UGrabbableComponent> GrabbableComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Relic")
	FDataTableRowHandle RelicData;

	UPROPERTY(ReplicatedUsing=OnRep_IsDisplayed, VisibleInstanceOnly, BlueprintReadOnly, Category="Relic")
	bool bIsDisplayed = true;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Relic")
	bool bIsUnlocked = true;

	UPROPERTY(ReplicatedUsing=OnRep_IsReturned, VisibleInstanceOnly, BlueprintReadOnly, Category="Relic|Delivery")
	bool bIsReturned = false;

	UPROPERTY(Transient)
	TObjectPtr<APlayerState> LastCarrierPlayerState;

	/** 이 Relic을 유효하게 촬영한 고유 플레이어 목록입니다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<APlayerState>> EvidencePhotographers;

	static constexpr int32 MaximumEvidencePhotographers = 6;
};
