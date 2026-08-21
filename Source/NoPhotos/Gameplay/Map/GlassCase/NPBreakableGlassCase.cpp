#include "Gameplay/Map/GlassCase/NPBreakableGlassCase.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "Gameplay/Interaction/Components/GrabbableComponent.h"
#include "Gameplay/Relic/Components/NPImpactReceiveComponent.h"
#include "Gameplay/Relic/NPBaseRelic.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogNPBreakableGlassCase, Log, All);

ANPBreakableGlassCase::ANPBreakableGlassCase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	GlassGeometryCollection = CreateDefaultSubobject<UGeometryCollectionComponent>(
		TEXT("GlassGeometryCollection"));
	GlassGeometryCollection->SetupAttachment(SceneRoot);
	GlassGeometryCollection->SetMobility(EComponentMobility::Movable);
	GlassGeometryCollection->SetEnableReplication(true);
	GlassGeometryCollection->SetReplicationAbandonAfterLevel(100);
	GlassGeometryCollection->SetReplicationMaxPositionAndVelocityCorrectionLevel(0);
	GlassGeometryCollection->ObjectType =
		EObjectStateTypeEnum::Chaos_Object_Dynamic;
	GlassGeometryCollection->SetCollisionObjectType(ECC_Destructible);
	GlassGeometryCollection->SetCollisionResponseToAllChannels(ECR_Block);
	GlassGeometryCollection->SetCollisionResponseToChannel(
		ECC_Destructible,
		ECR_Ignore);
	GlassGeometryCollection->SetNotifyRigidBodyCollision(true);
	GlassGeometryCollection->SetGenerateOverlapEvents(false);

	ImpactReceiveComponent = CreateDefaultSubobject<UNPImpactReceiveComponent>(
		TEXT("ImpactReceiveComponent"));
	ImpactReceiveComponent->SetImpactTargetComponent(GlassGeometryCollection);

	LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
	LidMesh->SetupAttachment(SceneRoot);
	LidMesh->SetMobility(EComponentMobility::Movable);
	LidMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	LidMesh->SetNotifyRigidBodyCollision(true);
	LidMesh->SetSimulatePhysics(false);
	LidMesh->SetVisibility(false, true);
	LidMesh->SetHiddenInGame(true, true);
	LidMesh->SetAngularDamping(LidAngularDamping);
	LidMesh->SetIsReplicated(true);

	LidHingeConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
		TEXT("LidHingeConstraint"));
	LidHingeConstraint->SetupAttachment(SceneRoot);
	LidHingeConstraint->SetDisableCollision(true);

	LidGrabbableComponent = CreateDefaultSubobject<UGrabbableComponent>(
		TEXT("LidGrabbableComponent"));
	LidGrabbableComponent->SetGrabEnabled(false);

	LockVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("LockVolume"));
	LockVolume->SetupAttachment(SceneRoot);
	LockVolume->SetBoxExtent(FVector(20.0f));
	LockVolume->SetCollisionObjectType(ECC_WorldDynamic);
	LockVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LockVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	LockVolume->SetGenerateOverlapEvents(true);
	LockVolume->SetHiddenInGame(true);

	RelicSpawnPoint = CreateDefaultSubobject<USceneComponent>(
		TEXT("RelicSpawnPoint"));
	RelicSpawnPoint->SetupAttachment(SceneRoot);
}

void ANPBreakableGlassCase::BeginPlay()
{
	Super::BeginPlay();

	LockVolume->OnComponentBeginOverlap.AddDynamic(
		this,
		&ANPBreakableGlassCase::HandleLockOverlap);
	ImpactReceiveComponent->OnDamaged.AddUObject(
		this,
		&ANPBreakableGlassCase::HandleDurabilityDamaged);
	ImpactReceiveComponent->OnDepleted.AddUObject(
		this,
		&ANPBreakableGlassCase::HandleDurabilityDepleted);

	if (GlassGeometryCollection->GetRestCollection())
	{
		if (HasAuthority())
		{
			GlassGeometryCollection->OnFullyDecayedEvent.AddDynamic(
				this,
				&ANPBreakableGlassCase::HandleFullyDecayed);
			GlassGeometryCollection->OnChaosBreakEvent.AddDynamic(
				this,
				&ANPBreakableGlassCase::HandleGeometryBreak);
		}

		GlassGeometryCollection->SetVisibility(true, true);
		GlassGeometryCollection->SetHiddenInGame(false, true);
		GlassGeometryCollection->ForceBrokenForCustomRenderer(true);
		GlassGeometryCollection->SetEnableDamageFromCollision(false);
		GlassGeometryCollection->SetNotifyBreaks(true);
		GlassGeometryCollection->SetCollisionEnabled(
			ECollisionEnabled::QueryAndPhysics);
		GlassGeometryCollection->SetEnableGravity(true);

		if (!IsOpened())
		{
			GlassGeometryCollection->SetSimulatePhysics(true);
			const int32 RootIndex = GlassGeometryCollection->GetRootIndex();
			if (RootIndex != INDEX_NONE)
			{
				// 설치형 오브젝트이므로 파괴되기 전에는 루트를 고정합니다.
				GlassGeometryCollection->SetAnchoredByIndex(RootIndex, true);
			}
		}
	}
	else
	{
		UE_LOG(
			LogNPBreakableGlassCase,
			Warning,
			TEXT("GlassGeometryCollection에 Rest Collection이 없습니다. Case=%s"),
			*GetNameSafe(this));
	}

	ApplyCaseState();

	if (HasAuthority())
	{
		SpawnContainedRelics();
	}
}

TArray<ANPBaseRelic*> ANPBreakableGlassCase::GetContainedRelics() const
{
	TArray<ANPBaseRelic*> Result;
	Result.Reserve(ContainedRelics.Num());
	for (ANPBaseRelic* Relic : ContainedRelics)
	{
		if (IsValid(Relic))
		{
			Result.Add(Relic);
		}
	}
	return Result;
}

void ANPBreakableGlassCase::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANPBreakableGlassCase, bIsBroken);
	DOREPLIFETIME(ANPBreakableGlassCase, bIsUnlocked);
	DOREPLIFETIME(ANPBreakableGlassCase, BreakLocation);
	DOREPLIFETIME(ANPBreakableGlassCase, ContainedRelics);
}

void ANPBreakableGlassCase::OnRep_IsBroken()
{
	// Multicast를 놓친 늦은 접속 클라이언트를 위한 보조 경로입니다.
	ApplyCaseState();
}

void ANPBreakableGlassCase::OnRep_IsUnlocked()
{
	ApplyCaseState();
}

void ANPBreakableGlassCase::OnRep_ContainedRelics()
{
	UpdateContainedRelicCollision();
}

void ANPBreakableGlassCase::HandleFullyDecayed()
{
	if (HasAuthority())
	{
		Destroy();
	}
}

void ANPBreakableGlassCase::MulticastBreakGlassCase_Implementation(
	const FVector_NetQuantize10 InBreakLocation)
{
	BreakLocation = InBreakLocation;
	bIsBroken = true;
	ApplyCaseState();
}

void ANPBreakableGlassCase::HandleDurabilityDamaged(
	const int32 Damage,
	const int32 CurrentHealth,
	const int32 MaxHealth)
{
	OnGlassCaseDamaged(Damage, CurrentHealth, MaxHealth);
	UE_LOG(
		LogNPBreakableGlassCase,
		Log,
		TEXT("유리 케이스 충격 피해: Case=%s, Damage=%d, Health=%d/%d"),
		*GetNameSafe(this),
		Damage,
		CurrentHealth,
		MaxHealth);
}

void ANPBreakableGlassCase::HandleDurabilityDepleted(
	const FVector& ImpactLocation)
{
	BreakGlassCase(ImpactLocation);
}

void ANPBreakableGlassCase::HandleGeometryBreak(
	const FChaosBreakEvent& BreakEvent)
{
	if (bActualBreakLogged)
	{
		return;
	}

	bActualBreakLogged = true;
	UE_LOG(
		LogNPBreakableGlassCase,
		Warning,
		TEXT("[GlassCase] 실제 Geometry Collection 파괴 확인: Case=%s, BoneIndex=%d, Location=%s, Mass=%.2f"),
		*GetNameSafe(this),
		BreakEvent.Index,
		*BreakEvent.Location.ToCompactString(),
		BreakEvent.Mass);
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

	return KeyActorClass.Get() != nullptr || !KeyActorTag.IsNone();
}

void ANPBreakableGlassCase::BreakGlassCase(
	const FVector& ImpactLocation)
{
	if (!HasAuthority() || IsOpened())
	{
		return;
	}

	BreakLocation = ImpactLocation;
	bIsBroken = true;
	MulticastBreakGlassCase(BreakLocation);
	UnlockContainedRelic();
	ForceNetUpdate();

	UE_LOG(
		LogNPBreakableGlassCase,
		Warning,
		TEXT("유리 케이스 파괴 확정: Case=%s, Location=%s"),
		*GetNameSafe(this),
		*FVector(BreakLocation).ToCompactString());
}

void ANPBreakableGlassCase::UnlockCase(AActor* KeyActor)
{
	if (!HasAuthority() || IsOpened() || !IsValid(KeyActor))
	{
		return;
	}

	bIsUnlocked = true;
	ApplyCaseState();
	UnlockContainedRelic();
	ForceNetUpdate();

	UE_LOG(
		LogNPBreakableGlassCase,
		Log,
		TEXT("열쇠로 유리 케이스 해제: Case=%s, Key=%s"),
		*GetNameSafe(this),
		*GetNameSafe(KeyActor));

	if (bConsumeKeyOnUnlock)
	{
		KeyActor->Destroy();
	}
}

void ANPBreakableGlassCase::ApplyCaseState()
{
	if (bIsBroken)
	{
		ApplyBrokenState();
		return;
	}

	if (bIsUnlocked)
	{
		LockVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LockVolume->SetVisibility(false, true);
		LockVolume->SetHiddenInGame(true, true);

		// 열쇠 해제 시 잠금 상태용 GC는 완전히 제거하고
		// 힌지로 움직이는 LidMesh만 표시합니다.
		GlassGeometryCollection->SetSimulatePhysics(false);
		GlassGeometryCollection->SetCollisionEnabled(
			ECollisionEnabled::NoCollision);
		GlassGeometryCollection->SetVisibility(false, true);
		GlassGeometryCollection->SetHiddenInGame(true, true);
		LidMesh->SetVisibility(true, true);
		LidMesh->SetHiddenInGame(false, true);
		LidMesh->SetCollisionProfileName(
			UCollisionProfile::PhysicsActor_ProfileName);
		LidMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		// 내부 유물의 Object Type이 PhysicsBody이므로 뚜껑 활성화 순간
		// 겹침 해소 힘으로 유물이 튀지 않게 서로의 물리 충돌만 무시합니다.
		LidMesh->SetCollisionResponseToChannel(
			ECC_PhysicsBody,
			ECR_Ignore);
		LidGrabbableComponent->SetGrabEnabled(true);
		LidMesh->SetAngularDamping(FMath::Max(0.0f, LidAngularDamping));
		LidMesh->SetSimulatePhysics(true);
		ConfigureLidHinge();
		LidMesh->WakeAllRigidBodies();

		if (!bUnlockedEventDispatched)
		{
			bUnlockedEventDispatched = true;
			OnGlassCaseUnlocked();
		}
	}
	else
	{
		LockVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		LockVolume->SetVisibility(true, true);
		LockVolume->SetHiddenInGame(false, true);
		GlassGeometryCollection->SetVisibility(true, true);
		GlassGeometryCollection->SetHiddenInGame(false, true);
		GlassGeometryCollection->SetCollisionEnabled(
			ECollisionEnabled::QueryAndPhysics);
		LidGrabbableComponent->SetGrabEnabled(false);
		LidMesh->SetSimulatePhysics(false);
		LidMesh->SetCollisionProfileName(
			UCollisionProfile::NoCollision_ProfileName);
		LidMesh->SetVisibility(false, true);
		LidMesh->SetHiddenInGame(true, true);
	}

	UpdateContainedRelicCollision();
}

void ANPBreakableGlassCase::ApplyBrokenState()
{
	LockVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LockVolume->SetVisibility(false, true);
	LockVolume->SetHiddenInGame(true, true);
	LidGrabbableComponent->SetGrabEnabled(false);
	LidHingeConstraint->BreakConstraint();
	LidMesh->SetSimulatePhysics(false);
	LidMesh->SetCollisionProfileName(
		UCollisionProfile::NoCollision_ProfileName);
	LidMesh->SetVisibility(false, true);
	LidMesh->SetHiddenInGame(true, true);
	UpdateContainedRelicCollision();

	if (!bBrokenEventDispatched)
	{
		bBrokenEventDispatched = true;
		OnGlassCaseBroken();
	}

	if (bClusterBreakApplied || !IsValid(GlassGeometryCollection))
	{
		return;
	}

	bClusterBreakApplied = true;
	GlassGeometryCollection->SetMobility(EComponentMobility::Movable);
	GlassGeometryCollection->SetVisibility(true, true);
	GlassGeometryCollection->SetHiddenInGame(false, true);
	GlassGeometryCollection->Activate(true);
	GlassGeometryCollection->SetCollisionEnabled(
		ECollisionEnabled::QueryAndPhysics);
	GlassGeometryCollection->SetCollisionObjectType(ECC_Destructible);
	GlassGeometryCollection->ObjectType =
		EObjectStateTypeEnum::Chaos_Object_Dynamic;
	GlassGeometryCollection->SetEnableGravity(true);
	GlassGeometryCollection->SetSimulatePhysics(true);
	GlassGeometryCollection->RemoveAllAnchors();
	GlassGeometryCollection->WakeAllRigidBodies();

	BreakRootCluster();
}

void ANPBreakableGlassCase::ConfigureLidHinge()
{
	LidHingeConstraint->SetDisableCollision(true);
	LidHingeConstraint->SetLinearXLimit(LCM_Locked, 0.0f);
	LidHingeConstraint->SetLinearYLimit(LCM_Locked, 0.0f);
	LidHingeConstraint->SetLinearZLimit(LCM_Locked, 0.0f);
	LidHingeConstraint->SetAngularSwing1Limit(
		ACM_Limited,
		FMath::Clamp(LidOpenAngle, 1.0f, 175.0f));
	LidHingeConstraint->SetAngularSwing2Limit(ACM_Locked, 0.0f);
	LidHingeConstraint->SetAngularTwistLimit(ACM_Locked, 0.0f);
	// 설치형 케이스이므로 힌지 프레임을 월드에 고정하고 뚜껑만 연결합니다.
	LidHingeConstraint->SetConstrainedComponents(
		nullptr,
		NAME_None,
		LidMesh,
		NAME_None);
}

void ANPBreakableGlassCase::BreakRootCluster()
{
	if (!IsValid(GlassGeometryCollection) || !bIsBroken)
	{
		return;
	}

	const int32 RootIndex = GlassGeometryCollection->GetRootIndex();
	if (RootIndex == INDEX_NONE)
	{
		UE_LOG(
			LogNPBreakableGlassCase,
			Warning,
			TEXT("유리 케이스 파괴 실패: RootIndex가 없습니다. Case=%s"),
			*GetNameSafe(this));
		return;
	}

	GlassGeometryCollection->CrumbleCluster(RootIndex);
}

void ANPBreakableGlassCase::SpawnContainedRelics()
{
	if (!HasAuthority() || !ContainedRelics.IsEmpty() ||
		!RelicBlueprintClass || RelicSpawnCount <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !RelicSpawnPoint)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform SpawnPointTransform = RelicSpawnPoint->GetComponentTransform();
	const FVector SafeSpawnScale(
		FMath::Max(0.01f, SpawnedRelicScale.X),
		FMath::Max(0.01f, SpawnedRelicScale.Y),
		FMath::Max(0.01f, SpawnedRelicScale.Z));
	const float CenterIndex = (static_cast<float>(RelicSpawnCount) - 1.0f) * 0.5f;

	for (int32 Index = 0; Index < RelicSpawnCount; ++Index)
	{
		FTransform SpawnTransform = SpawnPointTransform;
		const FVector LocalOffset(
			0.0f,
			(static_cast<float>(Index) - CenterIndex) *
				FMath::Max(0.0f, RelicSpawnSpacing),
			0.0f);
		SpawnTransform.SetLocation(
			SpawnPointTransform.TransformPositionNoScale(LocalOffset));
		SpawnTransform.SetScale3D(SafeSpawnScale);

		ANPBaseRelic* SpawnedRelic = World->SpawnActor<ANPBaseRelic>(
			RelicBlueprintClass,
			SpawnTransform,
			SpawnParameters);

		if (!IsValid(SpawnedRelic))
		{
			UE_LOG(
				LogNPBreakableGlassCase,
				Error,
				TEXT("케이스 내부 유물 생성 실패: Case=%s, Class=%s, Index=%d"),
				*GetNameSafe(this),
				*GetNameSafe(RelicBlueprintClass.Get()),
				Index);
			continue;
		}

		SpawnedRelic->SetUnlocked(IsOpened());
		ContainedRelics.Add(SpawnedRelic);
	}

	UpdateContainedRelicCollision();
	ForceNetUpdate();

	UE_LOG(
		LogNPBreakableGlassCase,
		Log,
		TEXT("케이스 내부 유물 생성 완료: Case=%s, Requested=%d, Spawned=%d"),
		*GetNameSafe(this),
		RelicSpawnCount,
		ContainedRelics.Num());
}

void ANPBreakableGlassCase::UnlockContainedRelic()
{
	if (!HasAuthority())
	{
		return;
	}

	for (ANPBaseRelic* Relic : ContainedRelics)
	{
		if (IsValid(Relic))
		{
			Relic->SetUnlocked(true);
		}
	}
	UpdateContainedRelicCollision();
}

void ANPBreakableGlassCase::UpdateContainedRelicCollision()
{
	for (ANPBaseRelic* Relic : ContainedRelics)
	{
		if (IsValid(Relic))
		{
			// 속이 빈 케이스가 하나의 Convex 충돌로 채워져 있어도
			// 내부 유물이 GC와 겹치며 튀지 않도록 Destructible만 무시합니다.
			if (UPrimitiveComponent* RelicPrimitive =
				Cast<UPrimitiveComponent>(Relic->GetRootComponent()))
			{
				RelicPrimitive->SetCollisionResponseToChannel(
					ECC_Destructible,
					ECR_Ignore);
			}
			Relic->SetActorEnableCollision(IsOpened());
		}
	}
}
