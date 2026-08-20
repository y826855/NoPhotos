#include "NPBreakableGlassCase.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "GameFramework/Pawn.h"
#include "Gameplay/Relic/NPBaseRelic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogNPBreakableGlassCase, Log, All);

ANPBreakableGlassCase::ANPBreakableGlassCase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(SceneRoot);

	CaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CaseMesh"));
	CaseMesh->SetupAttachment(SceneRoot);
	CaseMesh->SetMobility(EComponentMobility::Static);
	CaseMesh->SetCollisionProfileName(TEXT("BlockAll"));
	// 파괴 후 생성되는 유리 파편이 고정 프레임과 겹치며 튀어 오르는 것을 방지합니다.
	CaseMesh->SetCollisionResponseToChannel(ECC_Destructible, ECR_Ignore);
	CaseMesh->SetNotifyRigidBodyCollision(true);

	GlassCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("GlassCollision"));
	GlassCollision->SetupAttachment(SceneRoot);
	GlassCollision->SetBoxExtent(FVector(50.0f, 10.0f, 50.0f));
	GlassCollision->SetMobility(EComponentMobility::Static);
	GlassCollision->SetCollisionProfileName(TEXT("BlockAll"));
	GlassCollision->SetCollisionResponseToChannel(ECC_Destructible, ECR_Ignore);
	GlassCollision->SetNotifyRigidBodyCollision(true);
	GlassCollision->SetGenerateOverlapEvents(false);

	LockVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("LockVolume"));
	LockVolume->SetupAttachment(SceneRoot);
	LockVolume->SetMobility(EComponentMobility::Static);
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

	GeometryCollectionComponent = FindComponentByClass<UGeometryCollectionComponent>();
	if (GeometryCollectionComponent)
	{
		// Root Proxy는 네트워크 복제를 지원하지 않으므로 액터 Multicast로
		// 각 월드에서 동일한 시각 파괴를 실행합니다.
		GeometryCollectionComponent->SetIsReplicated(false);
		GeometryCollectionComponent->SetEnableReplication(false);
		GeometryCollectionComponent->ForceBrokenForCustomRenderer(true);
		GeometryCollectionComponent->SetEnableDamageFromCollision(false);
		GeometryCollectionComponent->SetNotifyBreaks(true);
		GeometryCollectionComponent->bNotifyCollisions = true;
		GeometryCollectionComponent->SetNotifyRigidBodyCollision(true);
		GeometryCollectionComponent->InitialLinearVelocity = FVector::ZeroVector;
		GeometryCollectionComponent->InitialAngularVelocity = FVector::ZeroVector;
		GeometryCollectionComponent->SetVisibility(true, true);
		GeometryCollectionComponent->SetHiddenInGame(false, true);
		GeometryCollectionComponent->OnChaosBreakEvent.AddDynamic(
			this,
			&ANPBreakableGlassCase::HandleChaosBreak);
		GeometryCollectionComponent->OnChaosPhysicsCollision.AddDynamic(
			this,
			&ANPBreakableGlassCase::HandleGlassPhysicsCollision);
		GeometryCollectionComponent->SetSimulatePhysics(false);
		GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		UE_LOG(
			LogNPBreakableGlassCase,
			Warning,
			TEXT("Geometry Collection Component가 없습니다: Case=%s"),
			*GetNameSafe(this));
	}

	GlassCollision->OnComponentHit.AddDynamic(this, &ANPBreakableGlassCase::HandleCaseHit);
	LockVolume->OnComponentBeginOverlap.AddDynamic(this, &ANPBreakableGlassCase::HandleLockOverlap);
	ApplyCaseState();
	ApplyGlassDestruction();
}

void ANPBreakableGlassCase::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANPBreakableGlassCase, bIsBroken);
	DOREPLIFETIME(ANPBreakableGlassCase, bIsUnlocked);
	DOREPLIFETIME(ANPBreakableGlassCase, BreakLocation);
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
		const FVector ImpactLocation = Hit.ImpactPoint.IsNearlyZero()
			? GlassCollision->GetComponentLocation()
			: FVector(Hit.ImpactPoint);
		BreakCase(ImpactStrength, ImpactLocation);
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

void ANPBreakableGlassCase::BreakCase(
	const float ImpactStrength,
	const FVector& ImpactLocation)
{
	if (!HasAuthority() || IsOpened())
	{
		return;
	}

	BreakLocation = ImpactLocation;
	bIsBroken = true;
	MulticastBreakGlass(BreakLocation);
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
	ApplyGlassDestruction();
}

void ANPBreakableGlassCase::MulticastBreakGlass_Implementation(
	const FVector_NetQuantize10 InBreakLocation)
{
	BreakLocation = InBreakLocation;
	bIsBroken = true;
	ApplyCaseState();
	ApplyGlassDestruction();

	UE_LOG(
		LogNPBreakableGlassCase,
		Warning,
		TEXT("유리 파괴 Multicast 수신: Case=%s, NetMode=%d, Location=%s"),
		*GetNameSafe(this),
		static_cast<int32>(GetNetMode()),
		*FVector(InBreakLocation).ToCompactString());
}

void ANPBreakableGlassCase::HandleChaosBreak(const FChaosBreakEvent& BreakEvent)
{
	UE_LOG(
		LogNPBreakableGlassCase,
		Verbose,
		TEXT("유리 파편 분리: Case=%s, BoneIndex=%d, Location=%s"),
		*GetNameSafe(this),
		BreakEvent.Index,
		*BreakEvent.Location.ToCompactString());
}

void ANPBreakableGlassCase::HandleGlassPhysicsCollision(
	const FChaosPhysicsCollisionInfo& CollisionInfo)
{
	if (!bIsBroken)
	{
		return;
	}

	UPrimitiveComponent* OtherComponent = CollisionInfo.OtherComponent.Get();
	AActor* OtherActor = OtherComponent ? OtherComponent->GetOwner() : nullptr;
	APawn* Pawn = Cast<APawn>(OtherActor);
	ANPBaseRelic* Relic = Cast<ANPBaseRelic>(OtherActor);
	if (!IsValid(Pawn) && !IsValid(Relic))
	{
		return;
	}
	if ((Pawn && ShardPushStrength <= 0.0f)
		|| (Relic && RelicShardPushStrength <= 0.0f))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double CurrentTime = World->GetTimeSeconds();
	const TWeakObjectPtr<AActor> ActorKey(OtherActor);
	const float EffectiveCooldown = Relic ? RelicShardPushCooldown : ShardPushCooldown;
	if (const double* LastPushTime = LastShardPushTimes.Find(ActorKey);
		LastPushTime && CurrentTime - *LastPushTime < FMath::Max(0.0f, EffectiveCooldown))
	{
		return;
	}
	LastShardPushTimes.FindOrAdd(ActorKey) = CurrentTime;

	FVector PushDirection = OtherComponent->GetPhysicsLinearVelocity().GetSafeNormal2D();
	if (PushDirection.IsNearlyZero())
	{
		PushDirection = OtherActor->GetVelocity().GetSafeNormal2D();
	}
	if (PushDirection.IsNearlyZero())
	{
		PushDirection = (CollisionInfo.Location - OtherActor->GetActorLocation()).GetSafeNormal2D();
	}
	if (PushDirection.IsNearlyZero())
	{
		return;
	}

	const float EffectiveRadius = FMath::Max(1.0f, ShardPushRadius);
	if (Relic)
	{
		// 접촉한 파편이 유물의 어느 쪽에 있는지 판단해 진행 방향의 좌우로 밀어냅니다.
		const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, PushDirection).GetSafeNormal();
		const FVector ContactOffset = (CollisionInfo.Location - Relic->GetActorLocation()).GetSafeNormal2D();
		float SideSign = FMath::Sign(FVector::DotProduct(ContactOffset, RightDirection));
		if (FMath::IsNearlyZero(SideSign))
		{
			SideSign = FMath::Sign(FVector::DotProduct(CollisionInfo.Normal, RightDirection));
		}
		if (FMath::IsNearlyZero(SideSign))
		{
			SideSign = 1.0f;
		}

		const FVector SideDirection = RightDirection * SideSign;
		const FVector SideImpulseOrigin = CollisionInfo.Location - SideDirection * (EffectiveRadius * 0.5f);
		GeometryCollectionComponent->AddRadialImpulse(
			SideImpulseOrigin,
			EffectiveRadius,
			FMath::Max(0.0f, RelicShardPushStrength),
			ERadialImpulseFalloff::RIF_Linear,
			true);
		return;
	}

	// 캐릭터는 충돌점 뒤에서 방사형으로 밀어 진행 방향과 양옆을 함께 비웁니다.
	const FVector ImpulseOrigin = CollisionInfo.Location - PushDirection * (EffectiveRadius * 0.5f);
	GeometryCollectionComponent->AddRadialImpulse(
		ImpulseOrigin,
		EffectiveRadius,
		FMath::Max(0.0f, ShardPushStrength),
		ERadialImpulseFalloff::RIF_Linear,
		true);
}

void ANPBreakableGlassCase::ApplyCaseState()
{
	const bool bOpened = IsOpened();
	GlassCollision->SetCollisionEnabled(
		bOpened ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	LockVolume->SetCollisionEnabled(
		bOpened ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);

	if (bDisableCaseCollisionWhenOpened)
	{
		CaseMesh->SetCollisionEnabled(
			bOpened ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}

	if (GeometryCollectionComponent && bIsUnlocked && !bIsBroken)
	{
		GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GeometryCollectionComponent->SetVisibility(false, true);
	}
}

void ANPBreakableGlassCase::ApplyGlassDestruction()
{
	if (!bIsBroken || bDestructionApplied)
	{
		return;
	}

	if (!GeometryCollectionComponent)
	{
		GeometryCollectionComponent = FindComponentByClass<UGeometryCollectionComponent>();
	}
	if (!GeometryCollectionComponent)
	{
		UE_LOG(
			LogNPBreakableGlassCase,
			Error,
			TEXT("유리 디스트럭션 실패: Geometry Collection이 없습니다. Case=%s"),
			*GetNameSafe(this));
		return;
	}

	bDestructionApplied = true;
	// Chaos 파편을 활성화하기 전에 기존 유리 충돌을 완전히 제거해야
	// 동일한 공간의 두 물리 형상이 서로를 밀어내지 않습니다.
	GlassCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GlassCollision->SetGenerateOverlapEvents(false);
	CaseMesh->SetCollisionResponseToChannel(ECC_Destructible, ECR_Ignore);
	GeometryCollectionComponent->SetMobility(EComponentMobility::Movable);
	GeometryCollectionComponent->SetVisibility(true, true);
	GeometryCollectionComponent->SetHiddenInGame(false, true);
	GeometryCollectionComponent->Activate(true);
	GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GeometryCollectionComponent->SetCollisionObjectType(ECC_Destructible);
	GeometryCollectionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	GeometryCollectionComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	GeometryCollectionComponent->ObjectType = EObjectStateTypeEnum::Chaos_Object_Dynamic;
	GeometryCollectionComponent->SetEnableGravity(true);
	GeometryCollectionComponent->SetSimulatePhysics(true);
	GeometryCollectionComponent->SetPhysicsLinearVelocity(FVector::ZeroVector, false);
	GeometryCollectionComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector, false);
	GeometryCollectionComponent->WakeAllRigidBodies();

	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&ANPBreakableGlassCase::ApplyDestructionStrain);
}

void ANPBreakableGlassCase::ApplyDestructionStrain()
{
	if (!IsValid(GeometryCollectionComponent) || !bIsBroken)
	{
		return;
	}

	const int32 RootIndex = GeometryCollectionComponent->GetRootIndex();
	if (RootIndex == INDEX_NONE)
	{
		UE_LOG(
			LogNPBreakableGlassCase,
			Error,
			TEXT("유리 디스트럭션 실패: RootIndex가 없습니다. Case=%s"),
			*GetNameSafe(this));
		return;
	}

	// 고정 케이스이므로 활성화 과정에서 전달된 속도는 파쇄 전에 제거합니다.
	GeometryCollectionComponent->SetPhysicsLinearVelocity(FVector::ZeroVector, false);
	GeometryCollectionComponent->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector, false);

	GeometryCollectionComponent->ApplyExternalStrain(
		RootIndex,
		FVector(BreakLocation),
		FMath::Max(0.0f, StrainRadius),
		FMath::Max(0, StrainPropagationDepth),
		FMath::Clamp(StrainPropagationFactor, 0.0f, 1.0f),
		FMath::Max(0.0f, DestructionStrain));

	UE_LOG(
		LogNPBreakableGlassCase,
		Warning,
		TEXT("유리 디스트럭션 적용: Case=%s, NetMode=%d, Strain=%.1f, Radius=%.1f"),
		*GetNameSafe(this),
		static_cast<int32>(GetNetMode()),
		DestructionStrain,
		StrainRadius);
}
