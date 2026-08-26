#include "Gameplay/AbilitySystem/NPAbilitySystemComponent.h"

#include "Core/GameplayTag/NPGameplayTags.h"
#include "EnhancedInputComponent.h"
#include "GameplayAbilitySpec.h"
#include "Gameplay/Relic/Components/NPUsableRelicComponent.h"
#include "InputAction.h"

UNPAbilitySystemComponent::UNPAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void UNPAbilitySystemComponent::InitializeForOwner()
{
	AActor* OwningActor = GetOwner();
	if (OwningActor)
	{
		InitAbilityActorInfo(OwningActor, OwningActor);
	}
}

void UNPAbilitySystemComponent::BindRelicUseInput(
	UEnhancedInputComponent* EnhancedInputComponent,
	UInputAction* RelicUseAction)
{
	if (!EnhancedInputComponent || !RelicUseAction)
	{
		return;
	}

	EnhancedInputComponent->BindAction(
		RelicUseAction,
		ETriggerEvent::Started,
		this,
		&UNPAbilitySystemComponent::ActivateRelicUseAbility);
}

void UNPAbilitySystemComponent::SetHeldRelic(AActor* Relic)
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return;
	}

	ClearHeldRelicAbility();
	if (!IsValid(Relic))
	{
		return;
	}

	const UNPUsableRelicComponent* UsableRelic =
		Relic->FindComponentByClass<UNPUsableRelicComponent>();
	if (!UsableRelic || !UsableRelic->GetUseAbilityClass())
	{
		return;
	}

	FGameplayAbilitySpec AbilitySpec(
		UsableRelic->GetUseAbilityClass(),
		1,
		INDEX_NONE,
		Relic);
	HeldRelicAbilityHandle = GiveAbility(AbilitySpec);
}

void UNPAbilitySystemComponent::ActivateRelicUseAbility()
{
	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(NPGameplayTags::Input_Relic_Use);
	TryActivateAbilitiesByTag(AbilityTags);
}

void UNPAbilitySystemComponent::ClearHeldRelicAbility()
{
	if (!HeldRelicAbilityHandle.IsValid())
	{
		return;
	}

	CancelAbilityHandle(HeldRelicAbilityHandle);
	ClearAbility(HeldRelicAbilityHandle);
	HeldRelicAbilityHandle = FGameplayAbilitySpecHandle();
}
