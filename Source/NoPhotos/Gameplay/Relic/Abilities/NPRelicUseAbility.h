#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "NPRelicUseAbility.generated.h"

UCLASS()
class NOPHOTOS_API UNPRelicUseAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UNPRelicUseAbility();

protected:
	virtual void ActivateAbility(
		FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
