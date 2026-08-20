#include "Gameplay/Relic/NPBreakableRelic.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Gameplay/Interaction/Components/GrabbableComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Net/UnrealNetwork.h"
#include "NoPhotos.h"
#include "TimerManager.h"

ANPBreakableRelic::ANPBreakableRelic()
{
	CollisionProxy = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionProxy"));
	CollisionProxy->SetBoxExtent(FVector(50.0f));
	CollisionProxy->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	CollisionProxy->SetCollisionResponseToChannel(ECC_Destructible, ECR_Ignore);
	CollisionProxy->SetNotifyRigidBodyCollision(true);
	CollisionProxy->SetSimulatePhysics(false);
	SetRootComponent(CollisionProxy);

	RelicMesh->SetupAttachment(CollisionProxy);
	RelicMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RelicMesh->SetSimulatePhysics(false);
	RelicMesh->SetVisibility(false, false);
}

void ANPBreakableRelic::BeginPlay()
{
	Super::BeginPlay();
	// 파괴 유물은 Static Mesh 대신 단순 CollisionProxy가 물리와 잡기를 담당합니다.
	RelicMesh->SetStaticMesh(nullptr);
	RelicMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RelicMesh->SetSimulatePhysics(false);
	RelicMesh->SetVisibility(false, false);
	// Grab Constraint가 OnGrabStarted보다 먼저 생성되므로 잡기 시도 전부터
	// CollisionProxy에 유효한 동적 물리 바디가 있어야 합니다.
	CollisionProxy->SetSimulatePhysics(true);

	GeometryCollectionComponent = FindComponentByClass<UGeometryCollectionComponent>();
	if (!GeometryCollectionComponent)
	{
		UE_LOG(
			LogNoPhotos,
			Warning,
			TEXT("[%s] Geometry Collection Component가 없습니다."),
			*GetNameSafe(this));
	}
	else
	{
		// 화면은 Geometry Collection, 잡기와 충격 감지는 CollisionProxy가 담당합니다.
		GeometryCollectionComponent->SetVisibility(true, true);
		GeometryCollectionComponent->SetHiddenInGame(false, true);
		// Geometry Collection의 Root Proxy는 동적으로 생성된 StaticMeshComponent라
		// NetGUID 복제를 지원하지 않습니다. Actor의 Multicast로 각 월드에서 동일한
		// 시각 파괴를 실행하므로 GC 자체의 Chaos 복제는 사용하지 않습니다.
		GeometryCollectionComponent->SetIsReplicated(false);
		GeometryCollectionComponent->SetEnableReplication(false);
		GeometryCollectionComponent->ForceBrokenForCustomRenderer(true);
		GeometryCollectionComponent->SetEnableDamageFromCollision(false);
		GeometryCollectionComponent->SetNotifyBreaks(true);
		GeometryCollectionComponent->OnChaosBreakEvent.AddDynamic(
			this,
			&ANPBreakableRelic::HandleChaosBreak);
		if (!bIsBroken)
		{
			GeometryCollectionComponent->SetSimulatePhysics(false);
			GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	ApplyBrokenState();
	CollisionProxy->OnComponentHit.AddDynamic(this, &ANPBreakableRelic::HandleRelicHit);
	GrabbableComponent->OnGrabStarted.AddUObject(
		this,
		&ANPBreakableRelic::HandleBreakableGrabStarted);
}

void ANPBreakableRelic::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANPBreakableRelic, bIsBroken);
	DOREPLIFETIME(ANPBreakableRelic, BreakLocation);
}

void ANPBreakableRelic::OnRep_IsBroken()
{
	// Multicast를 놓치는 늦은 접속 클라이언트를 위한 보조 경로입니다.
	ApplyBrokenState();
	if (bIsBroken)
	{
		ShowBrokenDebugMessage();
	}
}

void ANPBreakableRelic::MulticastBreakRelic_Implementation(
	const FVector_NetQuantize10 InBreakLocation)
{
	BreakLocation = InBreakLocation;
	bIsBroken = true;
	ApplyBrokenState();

	UE_LOG(
		LogNoPhotos,
		Warning,
		TEXT("[%s] 파괴 Multicast 수신: NetMode=%d, Location=%s"),
		*GetNameSafe(this),
		static_cast<int32>(GetNetMode()),
		*FVector(InBreakLocation).ToCompactString());
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
	if (const UWorld* World = GetWorld();
		World && World->GetTimeSeconds() < IgnoreImpactUntilTime)
	{
		return;
	}

	const float ImpactStrength = NormalImpulse.Size();
	if (ImpactStrength >= BreakImpactThreshold)
	{
		const FVector ImpactLocation = Hit.ImpactPoint.IsNearlyZero()
			? CollisionProxy->GetComponentLocation()
			: FVector(Hit.ImpactPoint);
		BreakRelic(ImpactStrength, ImpactLocation);
	}
}

void ANPBreakableRelic::HandleBreakableGrabStarted(
	UPrimitiveComponent* GrabbedComponent)
{
	if (const UWorld* World = GetWorld())
	{
		IgnoreImpactUntilTime = World->GetTimeSeconds()
			+ FMath::Max(0.0f, GrabImpactIgnoreDuration);
	}
	if (CollisionProxy && !CollisionProxy->IsSimulatingPhysics())
	{
		CollisionProxy->SetSimulatePhysics(true);
	}
}

void ANPBreakableRelic::HandleChaosBreak(const FChaosBreakEvent& BreakEvent)
{
	++ChaosBreakEventCount;
	UE_LOG(
		LogNoPhotos,
		Warning,
		TEXT("[%s] Chaos 실제 파편 분리 감지: Count=%d, BoneIndex=%d, Location=%s, Mass=%.2f"),
		*GetNameSafe(this),
		ChaosBreakEventCount,
		BreakEvent.Index,
		*BreakEvent.Location.ToCompactString(),
		BreakEvent.Mass);
}

void ANPBreakableRelic::BreakRelic(
	const float ImpactStrength,
	const FVector& ImpactLocation)
{
	if (!HasAuthority() || bIsBroken)
	{
		return;
	}

	BreakLocation = ImpactLocation;
	bIsBroken = true;
	MulticastBreakRelic(BreakLocation);
	ForceNetUpdate();
	ShowBrokenDebugMessage();

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
	if (!bIsBroken || bStrainApplied)
	{
		return;
	}

	if (!GeometryCollectionComponent)
	{
		GeometryCollectionComponent = FindComponentByClass<UGeometryCollectionComponent>();
	}
	if (!GeometryCollectionComponent)
	{
		const FString ErrorMessage = FString::Printf(
			TEXT("[BreakableRelic] %s: Geometry Collection을 찾지 못했습니다."),
			*GetNameSafe(this));
		UE_LOG(LogNoPhotos, Error, TEXT("%s"), *ErrorMessage);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Yellow, ErrorMessage);
		}
		return;
	}

	bStrainApplied = true;
	// 원인 분리 상태 유지: Geometry Collection을 RelicMesh에서 분리하지 않습니다.
	// GeometryCollectionComponent->DetachFromComponent(
	// 	FDetachmentTransformRules::KeepWorldTransform);
	GeometryCollectionComponent->SetMobility(EComponentMobility::Movable);
	GeometryCollectionComponent->SetVisibility(true, true);
	GeometryCollectionComponent->SetHiddenInGame(false, true);
	GeometryCollectionComponent->Activate(true);

	// CollisionProxy는 Actor의 물리 Root로 유지하고 Geometry Collection은 시각적 파괴를 담당합니다.

	GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GeometryCollectionComponent->SetCollisionObjectType(ECC_Destructible);
	GeometryCollectionComponent->ObjectType = EObjectStateTypeEnum::Chaos_Object_Dynamic;
	GeometryCollectionComponent->SetEnableGravity(true);
	// BeginPlay에서 정지시킨 Chaos 프록시에 이전 Kinematic 상태가 남지 않도록
	// Simulate 플래그를 먼저 켭니다.
	GeometryCollectionComponent->SetSimulatePhysics(true);
	// 원인 분리 테스트 1: 파괴 순간 Physics State 재생성이 정지를 유발하는지 확인합니다.
	// GeometryCollectionComponent->RecreatePhysicsState();
	GeometryCollectionComponent->WakeAllRigidBodies();

	// SetSimulatePhysics 직후에는 Chaos PhysicsProxy가 아직 준비되지 않을 수 있습니다.
	// 다음 틱에 Strain을 적용해야 호출이 유실되지 않습니다.
	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&ANPBreakableRelic::ApplyDestructionStrain);
}

void ANPBreakableRelic::ApplyDestructionStrain()
{
	if (!IsValid(GeometryCollectionComponent) || !bIsBroken)
	{
		return;
	}

	GeometryCollectionComponent->SetEnableGravity(true);
	GeometryCollectionComponent->SetSimulatePhysics(true);
	GeometryCollectionComponent->WakeAllRigidBodies();
	const int32 RootIndex = GeometryCollectionComponent->GetRootIndex();
	if (RootIndex == INDEX_NONE)
	{
		UE_LOG(
			LogNoPhotos,
			Error,
			TEXT("[%s] Geometry Collection RootIndex를 찾지 못해 Strain을 적용하지 못했습니다."),
			*GetNameSafe(this));
		return;
	}

	GeometryCollectionComponent->ApplyExternalStrain(
		RootIndex,
		FVector(BreakLocation),
		FMath::Max(0.0f, StrainRadius),
		FMath::Max(0, StrainPropagationDepth),
		FMath::Clamp(StrainPropagationFactor, 0.0f, 1.0f),
		FMath::Max(0.0f, DestructionStrain));

	FTimerHandle DestructionResultTimer;
	GetWorldTimerManager().SetTimer(
		DestructionResultTimer,
		this,
		&ANPBreakableRelic::ReportDestructionResult,
		0.5f,
		false);

	UE_LOG(
		LogNoPhotos,
		Warning,
		TEXT("[%s] Destruction 적용: NetMode=%d, GeometryCollection=%s, RootIndex=%d, Strain=%.1f, Radius=%.1f, CollisionProxyActive=%s"),
		*GetNameSafe(this),
		static_cast<int32>(GetNetMode()),
		*GetNameSafe(GeometryCollectionComponent),
		RootIndex,
		DestructionStrain,
		StrainRadius,
		CollisionProxy && CollisionProxy->IsCollisionEnabled() ? TEXT("true") : TEXT("false"));
}

void ANPBreakableRelic::ReportDestructionResult()
{
	if (ChaosBreakEventCount > 0)
	{
		UE_LOG(
			LogNoPhotos,
			Warning,
			TEXT("[%s] 디스트럭션 확인 완료: 실제 분리된 파편 이벤트=%d"),
			*GetNameSafe(this),
			ChaosBreakEventCount);
		return;
	}

	UE_LOG(
		LogNoPhotos,
		Error,
		TEXT("[%s] 디스트럭션 실패: Strain은 호출됐지만 Chaos Break 이벤트가 0개입니다."),
		*GetNameSafe(this));
}

void ANPBreakableRelic::ShowBrokenDebugMessage() const
{
	const FString Message = FString::Printf(
		TEXT("[BreakableRelic] %s 파괴되었습니다."),
		*GetNameSafe(this));

	UE_LOG(LogNoPhotos, Warning, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Red,
			Message);
	}
}
