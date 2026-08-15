#include "GrabbableComponent.h"

UGrabbableComponent::UGrabbableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGrabbableComponent::NotifyGrabStarted(UPrimitiveComponent* GrabbedComponent)
{
	OnGrabStarted.Broadcast(GrabbedComponent);
}
