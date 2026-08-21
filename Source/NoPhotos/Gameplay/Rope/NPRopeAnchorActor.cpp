#include "Gameplay/Rope/NPRopeAnchorActor.h"

#include "Components/SceneComponent.h"

ANPRopeAnchorActor::ANPRopeAnchorActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}
