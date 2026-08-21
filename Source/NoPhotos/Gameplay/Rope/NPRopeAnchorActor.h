#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPRopeAnchorActor.generated.h"

class USceneComponent;

/** Rope endpoint가 참조할 수 있는 가벼운 복제 Actor입니다. */
UCLASS(Blueprintable)
class NOPHOTOS_API ANPRopeAnchorActor : public AActor
{
	GENERATED_BODY()

public:
	ANPRopeAnchorActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;
};
