#include "Gameplay/Relic/Gimmick/NPPulleyBarrierGimmick.h"

#include "Components/SceneComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "Gameplay/Interaction/Components/GrabbableComponent.h"
#include "Gameplay/Relic/Gimmick/Components/NPPulleyBarrierGimmickComponent.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"

ANPPulleyBarrierGimmick::ANPPulleyBarrierGimmick()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bReplicates = true;
	NetUpdateFrequency = 30.0f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
	HandleMesh->SetupAttachment(SceneRoot);
	HandleMesh->SetMobility(EComponentMobility::Movable);
	HandleMesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	HandleMesh->SetEnableGravity(false);

	HandleConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(
		TEXT("HandleConstraint"));
	HandleConstraint->SetupAttachment(SceneRoot);
	HandleConstraint->SetDisableCollision(true);

	BarrierMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrierMesh"));
	BarrierMesh->SetupAttachment(SceneRoot);
	BarrierMesh->SetMobility(EComponentMobility::Movable);
	BarrierMesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);

	LeftPulleyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPulleyMesh"));
	LeftPulleyMesh->SetupAttachment(SceneRoot);
	LeftPulleyMesh->SetMobility(EComponentMobility::Movable);
	LeftPulleyMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	RightPulleyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightPulleyMesh"));
	RightPulleyMesh->SetupAttachment(SceneRoot);
	RightPulleyMesh->SetMobility(EComponentMobility::Movable);
	RightPulleyMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);

	GrabbableComponent = CreateDefaultSubobject<UGrabbableComponent>(
		TEXT("GrabbableComponent"));
	GimmickComponent = CreateDefaultSubobject<UNPPulleyBarrierGimmickComponent>(
		TEXT("GimmickComponent"));

	HandleWireMesh = CreateDefaultSubobject<USplineMeshComponent>(TEXT("HandleWireMesh"));
	HandleWireMesh->SetupAttachment(SceneRoot);
	HandleWireMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandleWireMesh->SetForwardAxis(ESplineMeshAxis::X);

	HandleWirePulleyAnchor = CreateDefaultSubobject<USceneComponent>(
		TEXT("HandleWirePulleyAnchor"));
	HandleWirePulleyAnchor->SetupAttachment(SceneRoot);

	HandleWireHandleAnchor = CreateDefaultSubobject<USceneComponent>(
		TEXT("HandleWireHandleAnchor"));
	HandleWireHandleAnchor->SetupAttachment(HandleMesh);

	TopWireMesh = CreateDefaultSubobject<USplineMeshComponent>(TEXT("TopWireMesh"));
	TopWireMesh->SetupAttachment(SceneRoot);
	TopWireMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TopWireMesh->SetForwardAxis(ESplineMeshAxis::X);

	TopWireLeftAnchor = CreateDefaultSubobject<USceneComponent>(
		TEXT("TopWireLeftAnchor"));
	TopWireLeftAnchor->SetupAttachment(SceneRoot);

	TopWireRightAnchor = CreateDefaultSubobject<USceneComponent>(
		TEXT("TopWireRightAnchor"));
	TopWireRightAnchor->SetupAttachment(SceneRoot);

	BarrierWireMesh = CreateDefaultSubobject<USplineMeshComponent>(TEXT("BarrierWireMesh"));
	BarrierWireMesh->SetupAttachment(SceneRoot);
	BarrierWireMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BarrierWireMesh->SetForwardAxis(ESplineMeshAxis::X);

	BarrierWirePulleyAnchor = CreateDefaultSubobject<USceneComponent>(
		TEXT("BarrierWirePulleyAnchor"));
	BarrierWirePulleyAnchor->SetupAttachment(SceneRoot);

	BarrierWireBarrierAnchor = CreateDefaultSubobject<USceneComponent>(
		TEXT("BarrierWireBarrierAnchor"));
	BarrierWireBarrierAnchor->SetupAttachment(BarrierMesh);
}

void ANPPulleyBarrierGimmick::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateWireMeshes();
}

void ANPPulleyBarrierGimmick::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPPulleyBarrierGimmick, ReplicatedTravelDistance);
}

float ANPPulleyBarrierGimmick::GetNormalizedTravel() const
{
	return HandleTravelDistance > UE_SMALL_NUMBER
		? FMath::Clamp(ReplicatedTravelDistance / HandleTravelDistance, 0.0f, 1.0f)
		: 0.0f;
}

bool ANPPulleyBarrierGimmick::IsOpened() const
{
	return GimmickComponent && GimmickComponent->IsCompleted();
}

void ANPPulleyBarrierGimmick::BeginPlay()
{
	Super::BeginPlay();
	HandleMesh->SetIsReplicated(false);

	InitialHandleRelativeLocation = HandleMesh->GetRelativeLocation();
	InitialHandleWorldLocation = HandleMesh->GetComponentLocation();
	InitialBarrierRelativeLocation = BarrierMesh->GetRelativeLocation();
	InitialLeftPulleyRelativeRotation = LeftPulleyMesh->GetRelativeRotation().Quaternion();
	InitialRightPulleyRelativeRotation = RightPulleyMesh->GetRelativeRotation().Quaternion();
	bInitialTransformsCached = true;

	GimmickComponent->OnCompleted.AddUObject(
		this,
		&ANPPulleyBarrierGimmick::HandleGimmickCompleted);
	GrabbableComponent->OnGrabStarted.AddUObject(
		this,
		&ANPPulleyBarrierGimmick::HandleGrabStarted);

	if (HasAuthority())
	{
		HandleMesh->SetSimulatePhysics(true);
		HandleMesh->SetEnableGravity(false);
		HandleMesh->SetLinearDamping(HandleLinearDamping);
		ConfigureHandleConstraint();
	}
	else
	{
		HandleMesh->SetSimulatePhysics(false);
	}

	ApplyTravel(ReplicatedTravelDistance);
}

void ANPPulleyBarrierGimmick::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || IsOpened())
	{
		return;
	}

	if (!GrabbableComponent->IsGrabbed()
		&& !HandleMesh->RigidBodyIsAwake())
	{
		UpdateTravelFromHandle();
		return;
	}

	const float SignedTravelDistance = GetSignedHandleTravelDistance();
	if (TrySettleHandle(SignedTravelDistance))
	{
		UpdateTravelFromHandle();
		return;
	}

	ApplyHandleReturnForce(SignedTravelDistance);
	LimitHandleDownwardSpeed();
	UpdateTravelFromHandle();
}

void ANPPulleyBarrierGimmick::ConfigureHandleConstraint()
{
	const FVector ConstraintLocation = HandleMesh->GetComponentLocation();
	HandleConstraint->SetWorldLocationAndRotation(
		ConstraintLocation,
		GetActorQuat());
	HandleConstraint->SetLinearXLimit(LCM_Locked, 0.0f);
	HandleConstraint->SetLinearYLimit(LCM_Locked, 0.0f);
	HandleConstraint->SetLinearZLimit(LCM_Limited, HandleTravelDistance);
	HandleConstraint->SetAngularSwing1Limit(ACM_Locked, 0.0f);
	HandleConstraint->SetAngularSwing2Limit(ACM_Locked, 0.0f);
	HandleConstraint->SetAngularTwistLimit(ACM_Locked, 0.0f);
	HandleConstraint->SetConstrainedComponents(
		nullptr,
		NAME_None,
		HandleMesh,
		NAME_None);
}

float ANPPulleyBarrierGimmick::GetSignedHandleTravelDistance() const
{
	return FVector::DotProduct(
		InitialHandleWorldLocation - HandleMesh->GetComponentLocation(),
		GetActorUpVector());
}

void ANPPulleyBarrierGimmick::ApplyHandleReturnForce(
	float SignedTravelDistance)
{
	if (CounterweightMass <= 0.0f
		|| ReturnForceRampDistance <= UE_SMALL_NUMBER
		|| !GetWorld())
	{
		return;
	}

	const float GravityAcceleration = FMath::Abs(GetWorld()->GetGravityZ());
	const float MaximumReturnForce = CounterweightMass * GravityAcceleration;
	const float SpringStrength = MaximumReturnForce / ReturnForceRampDistance;
	const float HandleMass = FMath::Max(HandleMesh->GetMass(), UE_SMALL_NUMBER);
	const float CriticalDamping = 2.0f * FMath::Sqrt(SpringStrength * HandleMass);
	const float TravelSpeed = -FVector::DotProduct(
		HandleMesh->GetPhysicsLinearVelocity(),
		GetActorUpVector());
	const bool bResistingDownwardPull = GrabbableComponent->IsGrabbed()
		&& (SignedTravelDistance > 0.0f || TravelSpeed > 0.0f);
	const float RestoringForce = bResistingDownwardPull
		? MaximumReturnForce
		: MaximumReturnForce * FMath::Clamp(
			SignedTravelDistance / ReturnForceRampDistance,
			-1.0f,
			1.0f);
	const float DampingForce = CriticalDamping
		* ReturnDampingRatio
		* TravelSpeed;

	HandleMesh->AddForce(
		GetActorUpVector() * (RestoringForce + DampingForce));
}

void ANPPulleyBarrierGimmick::LimitHandleDownwardSpeed()
{
	if (!GrabbableComponent->IsGrabbed()
		|| MaxHandleDownwardSpeed <= 0.0f)
	{
		return;
	}

	const FVector DownwardDirection = -GetActorUpVector();
	const FVector CurrentVelocity = HandleMesh->GetPhysicsLinearVelocity();
	const float DownwardSpeed = FVector::DotProduct(
		CurrentVelocity,
		DownwardDirection);
	if (DownwardSpeed <= MaxHandleDownwardSpeed)
	{
		return;
	}

	HandleMesh->SetPhysicsLinearVelocity(
		CurrentVelocity
		- DownwardDirection * (DownwardSpeed - MaxHandleDownwardSpeed));
}

bool ANPPulleyBarrierGimmick::TrySettleHandle(
	float SignedTravelDistance)
{
	if (GrabbableComponent->IsGrabbed())
	{
		return false;
	}

	const float TravelSpeed = -FVector::DotProduct(
		HandleMesh->GetPhysicsLinearVelocity(),
		GetActorUpVector());
	if (FMath::Abs(SignedTravelDistance) > SettlingDistance
		|| FMath::Abs(TravelSpeed) > SettlingSpeed)
	{
		return false;
	}

	HandleMesh->SetWorldLocation(
		InitialHandleWorldLocation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	HandleMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	HandleMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	HandleMesh->PutRigidBodyToSleep();
	return true;
}

void ANPPulleyBarrierGimmick::HandleGrabStarted(UPrimitiveComponent*)
{
	if (HasAuthority())
	{
		HandleMesh->WakeAllRigidBodies();
		OnHandleGrabbed.Broadcast();
	}
}

void ANPPulleyBarrierGimmick::UpdateTravelFromHandle()
{
	const float TravelDistance = FMath::Clamp(
		GetSignedHandleTravelDistance(),
		0.0f,
		HandleTravelDistance);

	if (!FMath::IsNearlyEqual(ReplicatedTravelDistance, TravelDistance, 0.1f))
	{
		ReplicatedTravelDistance = TravelDistance;
		ApplyTravel(ReplicatedTravelDistance);
	}

	if (GetNormalizedTravel() >= OpenTravelRatio)
	{
		ReplicatedTravelDistance = HandleTravelDistance;
		ApplyTravel(ReplicatedTravelDistance);
		GimmickComponent->CompleteGimmick();
		ForceNetUpdate();
	}
}

void ANPPulleyBarrierGimmick::ApplyTravel(float TravelDistance)
{
	if (!bInitialTransformsCached)
	{
		return;
	}

	const float ClampedTravelDistance = FMath::Clamp(
		TravelDistance,
		0.0f,
		HandleTravelDistance);
	BarrierMesh->SetRelativeLocation(
		InitialBarrierRelativeLocation
		+ FVector::UpVector * ClampedTravelDistance * BarrierTravelMultiplier);

	ApplyPulleyRotation(
		LeftPulleyMesh,
		InitialLeftPulleyRelativeRotation,
		ClampedTravelDistance,
		LeftPulleyRotationDirection);
	ApplyPulleyRotation(
		RightPulleyMesh,
		InitialRightPulleyRelativeRotation,
		ClampedTravelDistance,
		RightPulleyRotationDirection);

	if (!HasAuthority())
	{
		HandleMesh->SetRelativeLocation(
			InitialHandleRelativeLocation
			- FVector::UpVector * ClampedTravelDistance,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}

	UpdateWireMeshes();

	OnTravelChanged.Broadcast(
		HandleTravelDistance > UE_SMALL_NUMBER
			? ClampedTravelDistance / HandleTravelDistance
			: 0.0f);
}

void ANPPulleyBarrierGimmick::UpdateWireMeshes()
{
	UpdateWireMesh(
		HandleWireMesh,
		HandleWirePulleyAnchor,
		HandleWireHandleAnchor);
	UpdateWireMesh(
		TopWireMesh,
		TopWireLeftAnchor,
		TopWireRightAnchor);
	UpdateWireMesh(
		BarrierWireMesh,
		BarrierWirePulleyAnchor,
		BarrierWireBarrierAnchor);
}

void ANPPulleyBarrierGimmick::UpdateWireMesh(
	USplineMeshComponent* WireMesh,
	const USceneComponent* StartAnchor,
	const USceneComponent* EndAnchor)
{
	if (!WireMesh || !StartAnchor || !EndAnchor)
	{
		return;
	}

	const FTransform& WireTransform = WireMesh->GetComponentTransform();
	const FVector StartPosition = WireTransform.InverseTransformPosition(
		StartAnchor->GetComponentLocation());
	const FVector EndPosition = WireTransform.InverseTransformPosition(
		EndAnchor->GetComponentLocation());
	const FVector Tangent = EndPosition - StartPosition;

	WireMesh->SetStartAndEnd(
		StartPosition,
		Tangent,
		EndPosition,
		Tangent,
		true);
}

void ANPPulleyBarrierGimmick::ApplyPulleyRotation(
	UStaticMeshComponent* PulleyMesh,
	const FQuat& InitialRotation,
	float TravelDistance,
	float RotationDirection)
{
	if (!PulleyMesh || PulleyRotationAxis.IsNearlyZero() || PulleyRadius <= UE_SMALL_NUMBER)
	{
		return;
	}

	const float RotationRadians = TravelDistance / PulleyRadius * RotationDirection;
	const FQuat TravelRotation(
		PulleyRotationAxis.GetSafeNormal(),
		RotationRadians);
	PulleyMesh->SetRelativeRotation(InitialRotation * TravelRotation);
}

void ANPPulleyBarrierGimmick::HandleGimmickCompleted()
{
	ReplicatedTravelDistance = HandleTravelDistance;
	ApplyTravel(ReplicatedTravelDistance);
	OnBarrierOpened.Broadcast();
}

void ANPPulleyBarrierGimmick::OnRep_TravelDistance()
{
	ApplyTravel(ReplicatedTravelDistance);
}
