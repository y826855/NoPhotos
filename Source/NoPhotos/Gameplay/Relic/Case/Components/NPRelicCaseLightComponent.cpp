#include "Gameplay/Relic/Case/Components/NPRelicCaseLightComponent.h"

#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

UNPRelicCaseLightComponent::UNPRelicCaseLightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNPRelicCaseLightComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNPRelicCaseLightComponent, bLightEnabled);
	DOREPLIFETIME(UNPRelicCaseLightComponent, LightIntensity);
	DOREPLIFETIME(UNPRelicCaseLightComponent, LightColor);
}

void UNPRelicCaseLightComponent::SetLightEnabled(const bool bEnabled)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || bLightEnabled == bEnabled)
	{
		return;
	}

	bLightEnabled = bEnabled;
	ApplyLightState();
	Owner->ForceNetUpdate();
}

void UNPRelicCaseLightComponent::SetLightColor(
	const FLinearColor InLightColor)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || LightColor.Equals(InLightColor))
	{
		return;
	}

	LightColor = InLightColor;
	ApplyLightState();
	Owner->ForceNetUpdate();
}

void UNPRelicCaseLightComponent::SetLightIntensity(
	const float InLightIntensity)
{
	AActor* Owner = GetOwner();
	const float NewLightIntensity = FMath::Max(0.0f, InLightIntensity);
	if (!Owner || !Owner->HasAuthority() ||
		FMath::IsNearlyEqual(LightIntensity, NewLightIntensity))
	{
		return;
	}

	LightIntensity = NewLightIntensity;
	ApplyLightState();
	Owner->ForceNetUpdate();
}

void UNPRelicCaseLightComponent::OnRegister()
{
	Super::OnRegister();

	DynamicMaterialInstance = nullptr;
	ApplyLightState();
}

#if WITH_EDITOR
void UNPRelicCaseLightComponent::PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	LightIntensity = FMath::Max(0.0f, LightIntensity);
	DynamicMaterialInstance = nullptr;
	ApplyLightState();
}
#endif

void UNPRelicCaseLightComponent::OnRep_LightState()
{
	ApplyLightState();
}

void UNPRelicCaseLightComponent::ApplyLightState()
{
	if (!LightMaterialInstance || IntensityParameterName.IsNone() ||
		MaterialIndex < 0)
	{
		return;
	}

	if (!DynamicMaterialInstance)
	{
		DynamicMaterialInstance = CreateDynamicMaterialInstance(
			MaterialIndex,
			LightMaterialInstance);
	}

	if (DynamicMaterialInstance)
	{
		const float AppliedIntensity = bLightEnabled ? LightIntensity : 0.0f;
		DynamicMaterialInstance->SetScalarParameterValue(
			IntensityParameterName,
			AppliedIntensity);

		if (!ColorParameterName.IsNone())
		{
			DynamicMaterialInstance->SetVectorParameterValue(
				ColorParameterName,
				LightColor);
		}
	}
}
