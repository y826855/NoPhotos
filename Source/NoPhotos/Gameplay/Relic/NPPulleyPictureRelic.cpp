#include "Gameplay/Relic/NPPulleyPictureRelic.h"

#include "Gameplay/Relic/Gimmick/NPPulleyBarrierGimmick.h"

void ANPPulleyPictureRelic::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || !PulleyGimmick)
	{
		return;
	}

	PulleyGimmick->OnHandleGrabbed.AddUObject(
		this,
		&ANPPulleyPictureRelic::HandlePulleyHandleGrabbed);
}

void ANPPulleyPictureRelic::HandlePulleyHandleGrabbed()
{
	ReleaseFromDisplay();
}
