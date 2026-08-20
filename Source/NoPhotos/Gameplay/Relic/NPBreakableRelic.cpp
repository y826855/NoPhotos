#include "Gameplay/Relic/NPBreakableRelic.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "NoPhotos.h"

ANPBreakableRelic::ANPBreakableRelic()
{
	RelicMesh->SetNotifyRigidBodyCollision(true);
}

void ANPBreakableRelic::BeginPlay()
{
	Super::BeginPlay();

	if (!IntactMesh)
	{
		IntactMesh = RelicMesh->GetStaticMesh();
	}

	ApplyBrokenState();
	RelicMesh->OnComponentHit.AddDynamic(this, &ANPBreakableRelic::HandleRelicHit);
}

void ANPBreakableRelic::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANPBreakableRelic, bIsBroken);
}

void ANPBreakableRelic::OnRep_IsBroken()
{
	ApplyBrokenState();
}

void ANPBreakableRelic::HandleRelicHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	const FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!HasAuthority() || bIsBroken)
	{
		return;
	}

	const float ImpactStrength = NormalImpulse.Size();
	if (ImpactStrength >= BreakImpactThreshold)
	{
		BreakRelic(ImpactStrength);
	}
}

void ANPBreakableRelic::BreakRelic(const float ImpactStrength)
{
	if (!HasAuthority() || bIsBroken)
	{
		return;
	}

	bIsBroken = true;
	ApplyBrokenState();
	ForceNetUpdate();

	UE_LOG(
		LogNoPhotos,
		Log,
		TEXT("[%s] Relic broken. Impact=%.1f Threshold=%.1f"),
		*GetNameSafe(this),
		ImpactStrength,
		BreakImpactThreshold);
}

void ANPBreakableRelic::ApplyBrokenState()
{
	UStaticMesh* TargetMesh = bIsBroken ? BrokenMesh.Get() : IntactMesh.Get();
	if (TargetMesh)
	{
		RelicMesh->SetStaticMesh(TargetMesh);
		return;
	}

	if (bIsBroken)
	{
		UE_LOG(
			LogNoPhotos,
			Warning,
			TEXT("[%s] Relic is broken but BrokenMesh is not assigned."),
			*GetNameSafe(this));
	}
}
