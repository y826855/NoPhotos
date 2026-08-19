#include "NPVent.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "Gameplay/Character/NPStablePhysicsPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogNPVent, Log, All);

ANPVent::ANPVent()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	VentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VentMesh"));
	VentMesh->SetupAttachment(SceneRoot);

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(SceneRoot);
	InteractionVolume->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionVolume->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	InteractionVolume->SetGenerateOverlapEvents(true);

	ExitPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("ExitPoint"));
	ExitPoint->SetupAttachment(SceneRoot);
	ExitPoint->SetRelativeLocation(FVector(150.0f, 0.0f, 50.0f));
}

void ANPVent::BeginPlay()
{
	Super::BeginPlay();
	InteractionVolume->OnComponentBeginOverlap.AddDynamic(this, &ANPVent::HandleVentOverlap);

	if (IsValid(ConnectedVent)
		&& ConnectedVent != this
		&& !IsValid(ConnectedVent->ConnectedVent))
	{
		ConnectedVent->ConnectedVent = this;
	}
}

void ANPVent::HandleVentOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	APawn* InteractingPawn = Cast<APawn>(OtherActor);
	if (!HasAuthority()
		|| !IsValid(InteractingPawn)
		|| !IsValid(ConnectedVent)
		|| ConnectedVent == this
		|| IsTeleportBlockedFor(InteractingPawn))
	{
		return;
	}

	if (ANPStablePhysicsPawn* StablePhysicsPawn = Cast<ANPStablePhysicsPawn>(InteractingPawn))
	{
		StablePhysicsPawn->StopMovementInput();
	}

	const FTransform Destination = ConnectedVent->GetExitTransform();
	BlockTeleportFor(InteractingPawn);
	ConnectedVent->BlockTeleportFor(InteractingPawn);
	const bool bTeleported = InteractingPawn->SetActorLocationAndRotation(
		Destination.GetLocation(),
		Destination.Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	if (!bTeleported)
	{
		UE_LOG(
			LogNPVent,
			Warning,
			TEXT("벤트 이동 실패: Pawn=%s, From=%s, To=%s"),
			*GetNameSafe(InteractingPawn),
			*GetName(),
			*GetNameSafe(ConnectedVent));
		return;
	}

	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(InteractingPawn->GetRootComponent()))
	{
		RootPrimitive->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
		RootPrimitive->SetAllPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}

	if (AController* Controller = InteractingPawn->GetController())
	{
		Controller->SetControlRotation(Destination.Rotator());
	}

	UE_LOG(
		LogNPVent,
		Log,
		TEXT("벤트 이동 완료: Pawn=%s, From=%s, To=%s"),
		*GetNameSafe(InteractingPawn),
		*GetName(),
		*GetNameSafe(ConnectedVent));
}

FTransform ANPVent::GetExitTransform() const
{
	return IsValid(ExitPoint) ? ExitPoint->GetComponentTransform() : GetActorTransform();
}

bool ANPVent::IsTeleportBlockedFor(const APawn* Pawn) const
{
	if (!IsValid(Pawn) || !GetWorld())
	{
		return true;
	}

	const TWeakObjectPtr<APawn> PawnKey(const_cast<APawn*>(Pawn));
	const double* BlockedUntil = TeleportBlockedUntil.Find(PawnKey);
	return BlockedUntil && GetWorld()->GetTimeSeconds() < *BlockedUntil;
}

void ANPVent::BlockTeleportFor(APawn* Pawn)
{
	if (IsValid(Pawn) && GetWorld())
	{
		const TWeakObjectPtr<APawn> PawnKey(Pawn);
		TeleportBlockedUntil.FindOrAdd(PawnKey) = GetWorld()->GetTimeSeconds() + ArrivalBlockTime;
	}
}
