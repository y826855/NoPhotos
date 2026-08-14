#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrabbableComponent.generated.h"

UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class NOPHOTOS_API UGrabbableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGrabbableComponent();
};
