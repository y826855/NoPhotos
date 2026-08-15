#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrabbableComponent.generated.h"

class UPrimitiveComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGrabStarted, UPrimitiveComponent*);

UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class NOPHOTOS_API UGrabbableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrabbableComponent();

	void NotifyGrabStarted(UPrimitiveComponent* GrabbedComponent);

	FOnGrabStarted OnGrabStarted;
};
