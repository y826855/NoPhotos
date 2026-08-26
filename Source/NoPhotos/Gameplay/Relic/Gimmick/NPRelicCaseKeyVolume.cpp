#include "Gameplay/Relic/Gimmick/NPRelicCaseKeyVolume.h"

#include "Components/BoxComponent.h"
#include "Data/Interfaces/NPLockable.h"
#include "Gameplay/Relic/Gimmick/Components/NPGimmickProgressComponent.h"
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

	GimmickProgressComponent = CreateDefaultSubobject<UNPGimmickProgressComponent>(
		TEXT("GimmickProgressComponent"));
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
		&ANPRelicCaseKeyVolume::HandleLockBeginOverlap);
	LockVolume->OnComponentEndOverlap.AddDynamic(
		this,
		&ANPRelicCaseKeyVolume::HandleLockEndOverlap);
	GimmickProgressComponent->OnProgressCompleted.AddDynamic(
		this,
		&ANPRelicCaseKeyVolume::HandleProgressCompleted);
	GimmickProgressComponent->OnProgressLost.AddDynamic(
		this,
		&ANPRelicCaseKeyVolume::HandleProgressLost);
}

void ANPRelicCaseKeyVolume::HandleLockBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ANPRelicCaseKey* KeyActor = Cast<ANPRelicCaseKey>(OtherActor);
	if (!HasAuthority() || !IsValid(KeyActor) || IsValid(ActiveKey))
	{
		return;
	}

	ActiveKey = KeyActor;
	MulticastStartProgress();
}

void ANPRelicCaseKeyVolume::HandleLockEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	ANPRelicCaseKey* KeyActor = Cast<ANPRelicCaseKey>(OtherActor);
	if (!HasAuthority() || !IsValid(KeyActor) || KeyActor != ActiveKey ||
		LockVolume->IsOverlappingActor(KeyActor))
	{
		return;
	}

	ActiveKey = nullptr;
	MulticastReverseProgress();
}

void ANPRelicCaseKeyVolume::HandleProgressCompleted()
{
	RequestUnlock();
}

void ANPRelicCaseKeyVolume::HandleProgressLost()
{
	RequestLock();
}

void ANPRelicCaseKeyVolume::MulticastUnlockSucceeded_Implementation()
{
	OnUnlockSucceeded();
}

void ANPRelicCaseKeyVolume::MulticastStartProgress_Implementation()
{
	GimmickProgressComponent->StartProgress();
	OnKeyOverlapStarted();
}

void ANPRelicCaseKeyVolume::MulticastReverseProgress_Implementation()
{
	GimmickProgressComponent->ReverseProgress();
	OnKeyOverlapEnded();
}

void ANPRelicCaseKeyVolume::RequestUnlock()
{
	if (!HasAuthority() || !IsValid(ActiveKey) ||
		GimmickProgressComponent->GetProgress() < 1.0f ||
		!LockVolume->IsOverlappingActor(ActiveKey) ||
		!SetTargetActorsLocked(false))
	{
		return;
	}

	MulticastUnlockSucceeded();
	ActiveKey->NotifyUnlockSucceeded();
}

void ANPRelicCaseKeyVolume::RequestLock()
{
	if (HasAuthority())
	{
		SetTargetActorsLocked(true);
	}
}

bool ANPRelicCaseKeyVolume::SetTargetActorsLocked(const bool bLocked)
{
	bool bChangedAnyTarget = false;
	for (AActor* UnlockTarget : UnlockTargets)
	{
		if (IsValid(UnlockTarget) && UnlockTarget->GetClass()->ImplementsInterface(
			UNPLockable::StaticClass()))
		{
			bChangedAnyTarget |= INPLockable::Execute_TrySetLocked(
				UnlockTarget,
				bLocked);
		}
	}
	return bChangedAnyTarget;
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
