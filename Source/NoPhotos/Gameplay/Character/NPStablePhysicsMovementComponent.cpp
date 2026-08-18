#include "Gameplay/Character/NPStablePhysicsMovementComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

UNPStablePhysicsMovementComponent::UNPStablePhysicsMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UNPStablePhysicsMovementComponent::Initialize(
	USkeletalMeshComponent* InPhysicsMesh,
	float InCharacterForwardYawOffset)
{
	PhysicsMesh = InPhysicsMesh;
	CharacterForwardYawOffset = InCharacterForwardYawOffset;
}

void UNPStablePhysicsMovementComponent::ConfigureBoneNames(
	FName InPelvisBodyName,
	FName InLeftFootBoneName,
	FName InRightFootBoneName)
{
	PelvisBodyName = InPelvisBodyName;
	LeftFootBoneName = InLeftFootBoneName;
	RightFootBoneName = InRightFootBoneName;
}

void UNPStablePhysicsMovementComponent::SetTargetPelvisHeight(
	float InTargetPelvisHeight)
{
	TargetPelvisHeight = FMath::Max(InTargetPelvisHeight, 0.0f);
}

void UNPStablePhysicsMovementComponent::SetMaxMoveSpeed(float InMaxMoveSpeed)
{
	MaxMoveSpeed = FMath::Max(InMaxMoveSpeed, 0.0f);
}

void UNPStablePhysicsMovementComponent::SetJumpVelocityChange(
	float InJumpVelocityChange)
{
	JumpVelocityChange = FMath::Max(InJumpVelocityChange, 0.0f);
}

void UNPStablePhysicsMovementComponent::SetMoveInput(const FVector& InMoveInput)
{
	MoveInput = InMoveInput.GetClampedToMaxSize(1.0f);

	// 입력이 끝난 뒤에도 마지막 이동 방향을 바라보도록 0이 아닌 입력만 저장합니다.
	if (bOrientRotationToMovement && !MoveInput.IsNearlyZero())
	{
		FacingDirection = FVector(MoveInput.X, MoveInput.Y, 0.0f).GetSafeNormal();
		bHasFacingDirection = true;
	}
}

FVector UNPStablePhysicsMovementComponent::GetCurrentFacingDirection() const
{
	if (!PhysicsMesh)
	{
		return FVector::ForwardVector;
	}

	FVector CurrentForward = PhysicsMesh->GetForwardVector().RotateAngleAxis(
		CharacterForwardYawOffset,
		FVector::UpVector);
	CurrentForward.Z = 0.0f;
	return CurrentForward.GetSafeNormal();
}

void UNPStablePhysicsMovementComponent::SetAnimationStateOverride(
	bool bEnabled,
	const FVector& InVelocity,
	const FVector& InAcceleration,
	bool bInIsFalling)
{
	bUseAnimationStateOverride = bEnabled;
	AnimationVelocity = InVelocity;
	AnimationAcceleration = InAcceleration;
	bAnimationIsFalling = bInIsFalling;
}

void UNPStablePhysicsMovementComponent::RequestJump()
{
	bJumpRequested = true;
}

void UNPStablePhysicsMovementComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PhysicsMesh || !PhysicsMesh->IsSimulatingPhysics(PelvisBodyName))
	{
		Velocity = FVector::ZeroVector;
		CurrentAcceleration = FVector::ZeroVector;
		bGrounded = false;
		bIsFalling = true;
		bJumpRequested = false;
		return;
	}

	UpdateMovementState();
	UpdateGroundedState();
	if (!bPhysicsUpdatesEnabled)
	{
		bJumpRequested = false;
		return;
	}

	UpdateGroundSupportPhysics();
	UpdateMovementPhysics();
	UpdateFacingPhysics();
	UpdateBalancePhysics();
	UpdateJumpPhysics();
	bJumpRequested = false;
}

void UNPStablePhysicsMovementComponent::UpdateMovementState()
{
	Velocity = PhysicsMesh->GetPhysicsLinearVelocity(PelvisBodyName);
	CurrentAcceleration = FVector::ZeroVector;
}

void UNPStablePhysicsMovementComponent::UpdateGroundedState()
{
	bGrounded = IsFootGrounded(LeftFootBoneName) || IsFootGrounded(RightFootBoneName);
	bIsFalling = !bGrounded;
}

bool UNPStablePhysicsMovementComponent::IsFootGrounded(FName FootBoneName) const
{
	if (PhysicsMesh->GetBoneIndex(FootBoneName) == INDEX_NONE)
	{
		return false;
	}

	const FVector Start = PhysicsMesh->GetSocketLocation(FootBoneName) + FVector::UpVector * GroundProbeRadius;
	const FVector End = Start - FVector::UpVector * (GroundProbeRadius * 2.0f + 20.0f);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(StablePhysicsGround), false, GetOwner());
	FHitResult Hit;
	return GetWorld()->SweepSingleByObjectType(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(GroundProbeRadius),
		QueryParams);
}

bool UNPStablePhysicsMovementComponent::FindPelvisGroundDistance(float& OutGroundDistance) const
{
	const FVector Start = PhysicsMesh->GetSocketLocation(PelvisBodyName);
	const FVector End = Start - FVector::UpVector * GroundTraceDistance;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(StablePhysicsSupport), false, GetOwner());
	FHitResult Hit;
	if (!GetWorld()->SweepSingleByObjectType(
		Hit,
		Start,
		End,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(GroundProbeRadius),
		QueryParams))
	{
		return false;
	}

	OutGroundDistance = Hit.Distance + GroundProbeRadius;
	return true;
}

void UNPStablePhysicsMovementComponent::UpdateGroundSupportPhysics()
{
	float GroundDistance = 0.0f;
	if (!FindPelvisGroundDistance(GroundDistance))
	{
		return;
	}

	const float HeightError = TargetPelvisHeight - GroundDistance;
	if (HeightError <= 0.0f)
	{
		return;
	}

	const float SupportAcceleration = FMath::Clamp(
		HeightError * SupportStrength - Velocity.Z * SupportDamping,
		0.0f,
		MaxSupportAcceleration);
	const float TotalMass = FMath::Max(PhysicsMesh->GetMass(), 1.0f);
	PhysicsMesh->AddForce(FVector::UpVector * SupportAcceleration * TotalMass, PelvisBodyName);
}

void UNPStablePhysicsMovementComponent::UpdateMovementPhysics()
{
	FVector HorizontalVelocity = Velocity;
	HorizontalVelocity.Z = 0.0f;

	// 힘을 계속 누적하지 않고 현재 속도가 목표 속도에 가까워지도록 제어합니다.
	const FVector DesiredVelocity = MoveInput * MaxMoveSpeed;
	FVector MoveForce = (DesiredVelocity - HorizontalVelocity) * MoveStrength;
	MoveForce -= HorizontalVelocity * MoveDamping;
	MoveForce.Z = 0.0f;

	const float ControlMultiplier = bGrounded ? 1.0f : AirControlMultiplier;
	MoveForce = (MoveForce * ControlMultiplier).GetClampedToMaxSize(MaxMoveForce);
	PhysicsMesh->AddForce(MoveForce, PelvisBodyName);

	const float TotalMass = FMath::Max(PhysicsMesh->GetMass(), 1.0f);
	CurrentAcceleration = MoveForce / TotalMass;
}

void UNPStablePhysicsMovementComponent::UpdateFacingPhysics()
{
	if (!bOrientRotationToMovement || !bHasFacingDirection)
	{
		return;
	}

	const FVector CurrentForward = GetCurrentFacingDirection();
	if (CurrentForward.IsNearlyZero())
	{
		return;
	}

	const float YawError = FMath::Atan2(
		FVector::CrossProduct(CurrentForward, FacingDirection).Z,
		FVector::DotProduct(CurrentForward, FacingDirection));
	const float TargetAngularSpeed = FMath::Abs(FMath::RadiansToDegrees(YawError))
		> FacingStopTolerance
		? FMath::Clamp(
			YawError * FacingResponse,
			-MaxFacingAngularSpeed,
			MaxFacingAngularSpeed)
		: 0.0f;

	FVector AngularVelocity = PhysicsMesh->GetPhysicsAngularVelocityInRadians(PelvisBodyName);

	// 팔다리와 지면 접촉으로 생기는 회전 노이즈를 줄이기 위해 Yaw 각속도만 덮어씁니다.
	AngularVelocity.Z = TargetAngularSpeed;
	PhysicsMesh->SetPhysicsAngularVelocityInRadians(
		AngularVelocity,
		false,
		PelvisBodyName);
}

void UNPStablePhysicsMovementComponent::UpdateBalancePhysics()
{
	const FVector CurrentUp = PhysicsMesh->GetUpVector();
	const FVector TiltError = FVector::CrossProduct(CurrentUp, FVector::UpVector);
	FVector AngularVelocity = PhysicsMesh->GetPhysicsAngularVelocityInRadians(PelvisBodyName);
	AngularVelocity.Z = 0.0f;

	FVector BalanceTorque = TiltError * BalanceStrength - AngularVelocity * BalanceDamping;
	BalanceTorque.Z = 0.0f;
	BalanceTorque = BalanceTorque.GetClampedToMaxSize(MaxBalanceTorque);
	PhysicsMesh->AddTorqueInRadians(BalanceTorque, PelvisBodyName);
}

void UNPStablePhysicsMovementComponent::UpdateJumpPhysics()
{
	if (!bJumpRequested || !bGrounded)
	{
		return;
	}

	PhysicsMesh->AddImpulseToAllBodiesBelow(
		FVector::UpVector * JumpVelocityChange,
		PelvisBodyName,
		true,
		true);
	bGrounded = false;
	bIsFalling = true;
	OnJumpApplied.Broadcast();
}
