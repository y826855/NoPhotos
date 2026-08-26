#include "Gameplay/Relic/Abilities/NPRelicUseAbility.h"

#include "Core/GameplayTag/NPGameplayTags.h"
#include "Engine/Engine.h"

UNPRelicUseAbility::UNPRelicUseAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FGameplayTagContainer Tags;
	Tags.AddTag(NPGameplayTags::Input_Relic_Use);
	Tags.AddTag(NPGameplayTags::Ability_Relic);
	SetAssetTags(Tags);
}

void UNPRelicUseAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		TriggerEventData);

	if (ActorInfo && ActorInfo->IsLocallyControlled() && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Yellow,
			TEXT("Relic Use Ability Activated"));
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
