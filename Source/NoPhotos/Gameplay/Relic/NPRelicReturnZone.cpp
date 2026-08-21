#include "Gameplay/Relic/NPRelicReturnZone.h"

#include "Components/BoxComponent.h"
#include "Gameplay/Relic/NPBaseRelic.h"
#include "Gameplay/Relic/NPRelicDeliveryService.h"
#include "NoPhotosGameMode.h"

ANPRelicReturnZone::ANPRelicReturnZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	ReturnVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ReturnVolume"));
	SetRootComponent(ReturnVolume);
	ReturnVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ReturnVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	ReturnVolume->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	ReturnVolume->SetGenerateOverlapEvents(true);
}

void ANPRelicReturnZone::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		ReturnVolume->OnComponentBeginOverlap.AddDynamic(
			this,
			&ANPRelicReturnZone::HandleReturnVolumeBeginOverlap);
	}
}

void ANPRelicReturnZone::HandleReturnVolumeBeginOverlap(
	UPrimitiveComponent*,
	AActor* OtherActor,
	UPrimitiveComponent*,
	int32,
	bool,
	const FHitResult&)
{
	ANPBaseRelic* Relic = Cast<ANPBaseRelic>(OtherActor);
	ANoPhotosGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ANoPhotosGameMode>()
		: nullptr;
	UNPRelicDeliveryService* DeliveryService = GameMode
		? GameMode->GetRelicDeliveryService()
		: nullptr;
	if (Relic && DeliveryService)
	{
		DeliveryService->TryDeliverRelic(Relic, this);
	}
}
