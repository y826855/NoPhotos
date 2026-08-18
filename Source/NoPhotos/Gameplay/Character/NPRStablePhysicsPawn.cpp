#include "Gameplay/Character/NPRStablePhysicsPawn.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"
#include "Gameplay/Character/NPStablePhysicsGrabComponent.h"
#include "Gameplay/Character/NPStablePhysicsMovementComponent.h"

ANPRStablePhysicsPawn::ANPRStablePhysicsPawn()
{
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.0f);

	// 서버 권한 물리 결과를 소유 클라이언트에도 적용합니다.
	PhysicsMesh->bReplicatePhysicsToAutonomousProxy = true;
}

void ANPRStablePhysicsPawn::BeginPlay()
{
	Super::BeginPlay();

	const bool bServerAuthority = HasAuthority();
	PhysicsMovement->SetPhysicsUpdatesEnabled(bServerAuthority);
	RightHandGrab->SetGrabSimulationEnabled(bServerAuthority);
	if (!bServerAuthority)
	{
		PhysicsMovement->SetAnimationStateOverride(
			true,
			FVector(ReplicatedAnimationVelocity),
			FVector(ReplicatedAnimationAcceleration),
			bReplicatedAnimationIsFalling);
	}

	if (bServerAuthority)
	{
		RightHandGrab->OnGrabbedComponentChanged.AddUObject(
			this,
			&ANPRStablePhysicsPawn::HandleGrabbedComponentChanged);
	}
	else if (IsReplicatedGrabActive())
	{
		OnRep_GrabState();
	}
}

void ANPRStablePhysicsPawn::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANPRStablePhysicsPawn, bReplicatedRightHandActive);
	DOREPLIFETIME(ANPRStablePhysicsPawn, ReplicatedGrabState);
	DOREPLIFETIME(ANPRStablePhysicsPawn, ReplicatedServerHandWorldLocation);
	DOREPLIFETIME(ANPRStablePhysicsPawn, ReplicatedAnimationVelocity);
	DOREPLIFETIME(ANPRStablePhysicsPawn, ReplicatedAnimationAcceleration);
	DOREPLIFETIME(ANPRStablePhysicsPawn, bReplicatedAnimationIsFalling);
	DOREPLIFETIME(ANPRStablePhysicsPawn, ReplicatedAnimationForwardDirection);
}

void ANPRStablePhysicsPawn::Tick(float DeltaSeconds)
{
	if (!HasAuthority())
	{
		FVector ServerForward = FVector(ReplicatedAnimationForwardDirection);
		ServerForward.Z = 0.0f;
		ServerForward.Normalize();

		FVector ClientForward = GetActorForwardVector();
		ClientForward.Z = 0.0f;
		ClientForward.Normalize();

		const FVector ServerRight = FVector::CrossProduct(
			FVector::UpVector,
			ServerForward);
		const FVector ClientRight = FVector::CrossProduct(
			FVector::UpVector,
			ClientForward);
		const FVector ServerVelocity = FVector(ReplicatedAnimationVelocity);
		const FVector ServerAcceleration = FVector(ReplicatedAnimationAcceleration);
		const FVector ClientAnimationVelocity =
			ClientForward * FVector::DotProduct(ServerVelocity, ServerForward)
			+ ClientRight * FVector::DotProduct(ServerVelocity, ServerRight)
			+ FVector::UpVector * ServerVelocity.Z;
		const FVector ClientAnimationAcceleration =
			ClientForward * FVector::DotProduct(ServerAcceleration, ServerForward)
			+ ClientRight * FVector::DotProduct(ServerAcceleration, ServerRight)
			+ FVector::UpVector * ServerAcceleration.Z;

		PhysicsMovement->SetAnimationStateOverride(
			true,
			ClientAnimationVelocity,
			ClientAnimationAcceleration,
			bReplicatedAnimationIsFalling);
	}

	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		ReplicatedAnimationVelocity = PhysicsMovement->GetVelocity();
		ReplicatedAnimationAcceleration = PhysicsMovement->GetCurrentAcceleration();
		bReplicatedAnimationIsFalling = PhysicsMovement->GetIsFalling();
		FVector AnimationForward = GetActorForwardVector();
		AnimationForward.Z = 0.0f;
		ReplicatedAnimationForwardDirection = AnimationForward.GetSafeNormal();

		if (IsReplicatedGrabActive()
			&& PhysicsMesh->GetBoneIndex(RightHandBoneName) != INDEX_NONE)
		{
			ReplicatedServerHandWorldLocation =
				PhysicsMesh->GetSocketLocation(RightHandBoneName);
		}
		return;
	}

	if (IsReplicatedGrabActive())
	{
		DrawGrabNetworkDebug();
	}
}

void ANPRStablePhysicsPawn::ApplyMoveInput(const FVector& WorldMoveInput)
{
	const FVector ClampedMoveInput = WorldMoveInput.GetClampedToMaxSize(1.0f);
	if (HasAuthority())
	{
		Super::ApplyMoveInput(ClampedMoveInput);
		return;
	}

	if (!IsLocallyControlled())
	{
		return;
	}

	// 클라이언트 물리 Force는 꺼져 있지만 방향 디버그와 로컬 상태는 즉시 갱신합니다.
	Super::ApplyMoveInput(ClampedMoveInput);

	if (ClampedMoveInput.IsNearlyZero())
	{
		if (bClientWasMoving)
		{
			ServerStopMove();
			bClientWasMoving = false;
		}
		return;
	}

	bClientWasMoving = true;
	ServerSetMoveInput(ClampedMoveInput);
}

void ANPRStablePhysicsPawn::ApplyJumpRequest()
{
	if (HasAuthority())
	{
		Super::ApplyJumpRequest();
	}
	else if (IsLocallyControlled())
	{
		ServerRequestJump();
	}
}

void ANPRStablePhysicsPawn::ApplyRightHandState(bool bActive)
{
	if (HasAuthority())
	{
		SetServerRightHandState(bActive);
		return;
	}

	if (IsLocallyControlled())
	{
		// 소유 클라이언트는 서버 응답을 기다리지 않고 IK만 먼저 표시합니다.
		SetRightHandVisualState(bActive);
		ServerSetRightHandActive(bActive);
	}
}

void ANPRStablePhysicsPawn::ServerSetMoveInput_Implementation(
	FVector_NetQuantizeNormal WorldMoveInput)
{
	Super::ApplyMoveInput(FVector(WorldMoveInput));
}

void ANPRStablePhysicsPawn::ServerStopMove_Implementation()
{
	Super::ApplyMoveInput(FVector::ZeroVector);
}

void ANPRStablePhysicsPawn::ServerRequestJump_Implementation()
{
	Super::ApplyJumpRequest();
}

void ANPRStablePhysicsPawn::ServerSetRightHandActive_Implementation(bool bActive)
{
	SetServerRightHandState(bActive);
}

void ANPRStablePhysicsPawn::OnRep_RightHandActive()
{
	SetRightHandVisualState(bReplicatedRightHandActive);
}

void ANPRStablePhysicsPawn::OnRep_GrabState()
{
	if (!IsReplicatedGrabActive())
	{
		RightHandGrab->ClearReplicatedGrab();
		return;
	}

	UPrimitiveComponent* GrabbedComponent = ResolveReplicatedGrabbedComponent();
	if (!GrabbedComponent)
	{
		RightHandGrab->ClearReplicatedGrab();
		return;
	}

	RightHandGrab->ApplyReplicatedGrab(
		GrabbedComponent,
		ReplicatedGrabState.GrabbedBoneName,
		ReplicatedGrabState.ConstraintFrame1,
		ReplicatedGrabState.ConstraintFrame2,
		FVector(ReplicatedGrabState.GrabPointLocal));
}

void ANPRStablePhysicsPawn::HandleGrabbedComponentChanged(
	UPrimitiveComponent* NewGrabbedComponent)
{
	if (IsValid(NewGrabbedComponent))
	{
		ReplicatedGrabState.GrabbedActor = NewGrabbedComponent->GetOwner();
		ReplicatedGrabState.GrabbedComponentName = NewGrabbedComponent->GetFName();
		ReplicatedGrabState.GrabbedBoneName = RightHandGrab->GetGrabbedBoneName();
		ReplicatedGrabState.ConstraintFrame1 = RightHandGrab->GetGrabConstraintFrame(
			EConstraintFrame::Frame1);
		ReplicatedGrabState.ConstraintFrame2 = RightHandGrab->GetGrabConstraintFrame(
			EConstraintFrame::Frame2);
		ReplicatedGrabState.GrabPointLocal = RightHandGrab->GetGrabPointLocal();
		ReplicatedServerHandWorldLocation =
			PhysicsMesh->GetSocketLocation(RightHandBoneName);
	}
	else
	{
		ReplicatedGrabState = FReplicatedStableGrabState();
	}
	ForceNetUpdate();
}

UPrimitiveComponent* ANPRStablePhysicsPawn::ResolveReplicatedGrabbedComponent() const
{
	if (!IsValid(ReplicatedGrabState.GrabbedActor))
	{
		return nullptr;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	ReplicatedGrabState.GrabbedActor->GetComponents<UPrimitiveComponent>(
		PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent
			&& PrimitiveComponent->GetFName()
				== ReplicatedGrabState.GrabbedComponentName)
		{
			return PrimitiveComponent;
		}
	}

	return nullptr;
}

void ANPRStablePhysicsPawn::DrawGrabNetworkDebug() const
{
	if (!bDrawGrabNetworkDebug
		|| !GetWorld()
		|| PhysicsMesh->GetBoneIndex(RightHandBoneName) == INDEX_NONE)
	{
		return;
	}

	const FVector ServerHandLocation = FVector(ReplicatedServerHandWorldLocation);
	const FVector ClientHandLocation = PhysicsMesh->GetSocketLocation(RightHandBoneName);
	const float ErrorDistance = FVector::Distance(
		ServerHandLocation,
		ClientHandLocation);

	DrawDebugSphere(
		GetWorld(),
		ServerHandLocation,
		8.0f,
		12,
		FColor::Red,
		false,
		0.0f,
		0,
		2.0f);
	DrawDebugSphere(
		GetWorld(),
		ClientHandLocation,
		8.0f,
		12,
		FColor::Blue,
		false,
		0.0f,
		0,
		2.0f);
	DrawDebugLine(
		GetWorld(),
		ServerHandLocation,
		ClientHandLocation,
		FColor::Yellow,
		false,
		0.0f,
		0,
		2.0f);
	DrawDebugString(
		GetWorld(),
		(ServerHandLocation + ClientHandLocation) * 0.5f,
		FString::Printf(TEXT("Grab IK Error: %.1f cm"), ErrorDistance),
		nullptr,
		FColor::Yellow,
		0.0f,
		false,
		1.0f);
}

void ANPRStablePhysicsPawn::SetServerRightHandState(bool bActive)
{
	bReplicatedRightHandActive = bActive;
	Super::ApplyRightHandState(bActive);
	ForceNetUpdate();
}
