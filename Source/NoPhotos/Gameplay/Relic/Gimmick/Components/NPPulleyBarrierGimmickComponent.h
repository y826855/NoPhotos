#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Relic/Gimmick/Components/NPRelicGimmickComponent.h"
#include "NPPulleyBarrierGimmickComponent.generated.h"

UCLASS(ClassGroup=(Relic), meta=(BlueprintSpawnableComponent))
class NOPHOTOS_API UNPPulleyBarrierGimmickComponent
	: public UNPRelicGimmickComponent
{
	GENERATED_BODY()

public:
	UNPPulleyBarrierGimmickComponent();
};
