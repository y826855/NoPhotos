#include "Gameplay/Relic/Components/NPUsableRelicComponent.h"

#include "Gameplay/Relic/Abilities/NPRelicUseAbility.h"

UNPUsableRelicComponent::UNPUsableRelicComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	UseAbilityClass = UNPRelicUseAbility::StaticClass();
}
