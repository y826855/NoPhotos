#include "NPBreakableGlassCase.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogNPBreakableGlassCase, Log, All);

ANPBreakableGlassCase::ANPBreakableGlassCase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CaseMesh"));
	CaseMesh->SetupAttachment(SceneRoot);
	CaseMesh->SetMobility(EComponentMobility::Movable);
	CaseMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CaseMesh->SetNotifyRigidBodyCollision(true);

	LockVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("LockVolume"));
	LockVolume->SetupAttachment(SceneRoot);
	LockVolume->SetBoxExtent(FVector(20.0f, 20.0f, 20.0f));
	LockVolume->SetCollisionObjectType(ECC_WorldDynamic);
	LockVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LockVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	LockVolume->SetGenerateOverlapEvents(true);
	LockVolume->SetHiddenInGame(true);
}

void ANPBreakableGlassCase::BeginPlay()
{
	Super::BeginPlay();

	if (!IntactMesh)
	{
		IntactMesh = CaseMesh->GetStaticMesh();
	}

	CaseMesh->OnComponentHit.AddDynamic(this, &ANPBreakableGlassCase::HandleCaseHit);
	LockVolume->OnComponentBeginOverlap.AddDynamic(this, &ANPBreakableGlassCase::HandleLockOverlap);
	ApplyCaseState();
}

void ANPBreakableGlassCase::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANPBreakableGlassCase, bIsBroken);
	DOREPLIFETIME(ANPBreakableGlassCase, bIsUnlocked);
}

void ANPBreakableGlassCase::HandleCaseHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	const FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!HasAuthority() || IsOpened())
	{
		return;
	}

	const float ImpactStrength = NormalImpulse.Size();
	if (ImpactStrength >= BreakImpactThreshold)
	{
		BreakCase(ImpactStrength);
	}
}

void ANPBreakableGlassCase::HandleLockOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (HasAuthority() && !IsOpened() && IsValidKey(OtherActor))
	{
		UnlockCase(OtherActor);
	}
}

bool ANPBreakableGlassCase::IsValidKey(const AActor* KeyActor) const
{
	if (!IsValid(KeyActor) || KeyActor == this)
	{
		return false;
	}

	if (KeyActorClass && !KeyActor->IsA(KeyActorClass))
	{
		return false;
	}

	if (!KeyActorTag.IsNone() && !KeyActor->ActorHasTag(KeyActorTag))
	{
		return false;
	}

	// 클래스와 태그를 모두 비워 모든 액터가 열쇠가 되는 실수를 막습니다.
	return KeyActorClass.Get() != nullptr || !KeyActorTag.IsNone();
}

void ANPBreakableGlassCase::BreakCase(const float ImpactStrength)
{
	if (!HasAuthority() || IsOpened())
	{
		return;
	}

	bIsBroken = true;
	ApplyCaseState();
	ForceNetUpdate();

	UE_LOG(
		LogNPBreakableGlassCase,
		Log,
		TEXT("유리 케이스 파손: Case=%s, Impact=%.1f, Threshold=%.1f"),
		*GetName(),
		ImpactStrength,
		BreakImpactThreshold);
}

void ANPBreakableGlassCase::UnlockCase(AActor* KeyActor)
{
	if (!HasAuthority() || IsOpened() || !IsValid(KeyActor))
	{
		return;
	}

	bIsUnlocked = true;
	ApplyCaseState();
	ForceNetUpdate();

	UE_LOG(
		LogNPBreakableGlassCase,
		Log,
		TEXT("열쇠로 유리 케이스 해제: Case=%s, Key=%s"),
		*GetName(),
		*GetNameSafe(KeyActor));

	if (bConsumeKeyOnUnlock)
	{
		KeyActor->Destroy();
	}
}

void ANPBreakableGlassCase::OnRep_CaseState()
{
	ApplyCaseState();
}

void ANPBreakableGlassCase::ApplyCaseState()
{
	UStaticMesh* TargetMesh = IntactMesh.Get();
	if (bIsBroken && BrokenMesh)
	{
		TargetMesh = BrokenMesh.Get();
	}
	else if (bIsUnlocked && UnlockedMesh)
	{
		TargetMesh = UnlockedMesh.Get();
	}

	if (TargetMesh)
	{
		CaseMesh->SetStaticMesh(TargetMesh);
	}

	const bool bOpened = IsOpened();
	LockVolume->SetCollisionEnabled(
		bOpened ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);

	if (bDisableCaseCollisionWhenOpened)
	{
		CaseMesh->SetCollisionEnabled(
			bOpened ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}
}
