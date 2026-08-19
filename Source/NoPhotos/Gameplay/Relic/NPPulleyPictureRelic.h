#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Relic/NPBaseRelic.h"
#include "NPPulleyPictureRelic.generated.h"

class ANPPulleyBarrierGimmick;

UCLASS(Blueprintable)
class NOPHOTOS_API ANPPulleyPictureRelic : public ANPBaseRelic
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	UPROPERTY(
		EditInstanceOnly,
		BlueprintReadOnly,
		Category="Pulley Picture")
	TObjectPtr<ANPPulleyBarrierGimmick> PulleyGimmick;

private:
	void HandlePulleyHandleGrabbed();
};
