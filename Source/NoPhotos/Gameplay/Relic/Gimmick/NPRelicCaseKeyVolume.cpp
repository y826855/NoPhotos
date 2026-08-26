#include "Gameplay/Relic/Gimmick/NPRelicCaseKeyVolume.h"

#include "Components/BoxComponent.h"
#include "Data/Interfaces/NPLockableInterface.h"
#include "Gameplay/Relic/Gimmick/NPRelicCaseKey.h"

#if WITH_EDITOR
#include "DrawDebugHelpers.h"
#endif

ANPRelicCaseKeyVolume::ANPRelicCaseKeyVolume()
{
#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = true;
#else
	PrimaryActorTick.bCanEverTick = false;
#endif
	bReplicates = true;
	SetReplicateMovement(false);

	LockVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("LockVolume"));
	SetRootComponent(LockVolume);
	LockVolume->SetBoxExtent(FVector(20.0f));
	LockVolume->SetCollisionObjectType(ECC_WorldDynamic);
	LockVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LockVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	LockVolume->SetGenerateOverlapEvents(true);
}

void ANPRelicCaseKeyVolume::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		LockVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}

	LockVolume->OnComponentBeginOverlap.AddDynamic(
		this,
		&ANPRelicCaseKeyVolume::HandleLockOverlap);
}

void ANPRelicCaseKeyVolume::HandleLockOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ANPRelicCaseKey* KeyActor = Cast<ANPRelicCaseKey>(OtherActor);
	if (!HasAuthority() || !IsValid(KeyActor) || !UnlockTargetActors())
	{
		return;
	}

	LockVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MulticastUnlockSucceeded();
	KeyActor->NotifyUnlockSucceeded();
	if (bConsumeKeyOnUnlock)
	{
		KeyActor->Destroy();
	}
}

void ANPRelicCaseKeyVolume::MulticastUnlockSucceeded_Implementation()
{
	OnUnlockSucceeded();
}

bool ANPRelicCaseKeyVolume::UnlockTargetActors()
{
	bool bUnlockedAnyTarget = false;
	for (AActor* UnlockTarget : UnlockTargets)
	{
		if (IsValid(UnlockTarget) && UnlockTarget->GetClass()->ImplementsInterface(
			UNPLockableInterface::StaticClass()))
		{
			bUnlockedAnyTarget |= INPLockableInterface::Execute_TrySetLocked(
				UnlockTarget,
				false);
		}
	}
	return bUnlockedAnyTarget;
}

#pragma region Debug
#if WITH_EDITOR
void ANPRelicCaseKeyVolume::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	DrawDebugConnections();
}

bool ANPRelicCaseKeyVolume::ShouldTickIfViewportsOnly() const
{
	return true;
}

void ANPRelicCaseKeyVolume::DrawDebugConnections() const
{
#if WITH_EDITORONLY_DATA
	if (!bDrawDebugConnections || !GetWorld())
	{
		return;
	}

	const FVector StartLocation = LockVolume->GetComponentLocation();
	for (const AActor* UnlockTarget : UnlockTargets)
	{
		if (IsValid(UnlockTarget))
		{
			DrawDebugLine(
				GetWorld(),
				StartLocation,
				UnlockTarget->GetActorLocation(),
				DebugConnectionColor,
				false,
				0.0f,
				0,
				FMath::Max(0.0f, DebugConnectionThickness));
		}
	}
#endif
}
#endif
#pragma endregion Debug
