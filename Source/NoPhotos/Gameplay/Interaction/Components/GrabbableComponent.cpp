#include "GrabbableComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

UGrabbableComponent::UGrabbableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGrabbableComponent::SetGrabEnabled(bool bEnabled)
{
	bGrabEnabled = bEnabled;
}

UPrimitiveComponent* UGrabbableComponent::ResolveGrabTarget(
	UPrimitiveComponent* DetectedComponent) const
{
	if (AActor* Owner = GetOwner())
	{
		if (UPrimitiveComponent* RootPrimitive =
			Cast<UPrimitiveComponent>(Owner->GetRootComponent()))
		{
			return RootPrimitive;
		}
	}

	return DetectedComponent;
}

void UGrabbableComponent::NotifyGrabStarted(UPrimitiveComponent* GrabbedComponent)
{
	if (CanBeGrabbed())
	{
		bIsGrabbed = true;
		CurrentLinearGrabForce = FVector::ZeroVector;
		CurrentAngularGrabForce = FVector::ZeroVector;
		OnGrabStarted.Broadcast(GrabbedComponent);
	}
}

void UGrabbableComponent::NotifyGrabForce(
	const FVector& LinearForce,
	const FVector& AngularForce,
	float IntentForceAlignment)
{
	if (!bIsGrabbed)
	{
		return;
	}

	CurrentLinearGrabForce = LinearForce;
	CurrentAngularGrabForce = AngularForce;
	OnGrabForceUpdated.Broadcast(
		CurrentLinearGrabForce,
		CurrentAngularGrabForce,
		IntentForceAlignment);
}

void UGrabbableComponent::NotifyGrabEnded()
{
	if (!bIsGrabbed)
	{
		return;
	}

	bIsGrabbed = false;
	CurrentLinearGrabForce = FVector::ZeroVector;
	CurrentAngularGrabForce = FVector::ZeroVector;
	OnGrabEnded.Broadcast();
}
