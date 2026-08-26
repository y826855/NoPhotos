#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NPUsableRelicComponent.generated.h"

class UGameplayAbility;

USTRUCT(BlueprintType)
struct NOPHOTOS_API FNPRelicSwingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swing", meta=(ClampMin="0.01", Units="s"))
	float Duration = 0.5f;

	/** 부호에 따라 회전 방향이 결정됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swing")
	float Torque = 750000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swing", meta=(ClampMin="0.0", Units="deg/s"))
	float MaxAngularSpeed = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swing|Physics")
	bool bDisableHeldRelicGravity = true;

	/** 0이면 휘두르는 동안 질량을 변경하지 않습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swing|Physics", meta=(ClampMin="0.0", Units="kg"))
	float HeldRelicMass = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swing|Physical Animation", meta=(ClampMin="0.0"))
	float ArmOrientationStrength = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swing|Physical Animation", meta=(ClampMin="0.0"))
	float ArmAngularVelocityStrength = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swing|Physical Animation", meta=(ClampMin="0.0"))
	float HandOrientationStrength = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swing|Physical Animation", meta=(ClampMin="0.0"))
	float HandAngularVelocityStrength = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Swing|Grab")
	bool bPreventGrabConstraintBreak = true;
};

UCLASS(ClassGroup=(Relic), meta=(BlueprintSpawnableComponent))
class NOPHOTOS_API UNPUsableRelicComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNPUsableRelicComponent();

	TSubclassOf<UGameplayAbility> GetUseAbilityClass() const
	{
		return UseAbilityClass;
	}
	const FNPRelicSwingSettings& GetSwingSettings() const
	{
		return SwingSettings;
	}

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Relic Ability", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UGameplayAbility> UseAbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Relic Ability", meta=(AllowPrivateAccess="true"))
	FNPRelicSwingSettings SwingSettings;
};
