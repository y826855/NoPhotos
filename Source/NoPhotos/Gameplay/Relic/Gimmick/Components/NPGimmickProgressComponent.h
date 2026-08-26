#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPGimmickProgressComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnGimmickProgressChanged,
	float,
	Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGimmickProgressCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGimmickProgressLost);

UCLASS(ClassGroup=(Gimmick), meta=(BlueprintSpawnableComponent))
class NOPHOTOS_API UNPGimmickProgressComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPGimmickProgressComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Gimmick Progress")
	void StartProgress();

	UFUNCTION(BlueprintCallable, Category = "Gimmick Progress")
	void ReverseProgress();

	UFUNCTION(BlueprintPure, Category = "Gimmick Progress")
	float GetProgress() const { return CurrentProgress; }

	UPROPERTY(BlueprintAssignable, Category = "Gimmick Progress")
	FOnGimmickProgressChanged OnProgressChanged;

	UPROPERTY(BlueprintAssignable, Category = "Gimmick Progress")
	FOnGimmickProgressCompleted OnProgressCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Gimmick Progress")
	FOnGimmickProgressLost OnProgressLost;

protected:
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Gimmick Progress",
		meta = (ClampMin = "0.01", Units = "s"))
	float ProgressDuration = 1.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Gimmick Progress",
		meta = (ClampMin = "0.01", Units = "s"))
	float ReverseDuration = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Gimmick Progress")
	float CurrentProgress = 0.0f;

private:
	float ProgressDirection = 0.0f;
};
