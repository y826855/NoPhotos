#include "Gameplay/Character/NPRStablePhysicsPawn.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Gameplay/Character/NPStablePhysicsGrabComponent.h"
#include "Gameplay/Character/NPStablePhysicsMovementComponent.h"

ANPRStablePhysicsPawn::ANPRStablePhysicsPawn()
{
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.0f);

	// 소유 클라이언트가 골반 Root Body 상태를 결정합니다.
	PhysicsMesh->bReplicatePhysicsToAutonomousProxy = false;
}

void ANPRStablePhysicsPawn::BeginPlay()
{
	Super::BeginPlay();

	const bool bServerAuthority = HasAuthority();
	const bool bRunsMovementPhysics = bServerAuthority || IsLocallyControlled();
	PhysicsMovement->SetPhysicsUpdatesEnabled(bRunsMovementPhysics);
	RightHandGrab->SetGrabSimulationEnabled(bServerAuthority);
	if (!bRunsMovementPhysics)
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
	DOREPLIFETIME_CONDITION(
		ANPRStablePhysicsPawn,
		ReplicatedViewRotation,
		COND_SkipOwner);
}

void ANPRStablePhysicsPawn::Tick(float DeltaSeconds)
{
	UpdateViewRotationReplication(DeltaSeconds);

	if (!HasAuthority())
	{
		const bool bRunsMovementPhysics = IsLocallyControlled();
		PhysicsMovement->SetPhysicsUpdatesEnabled(bRunsMovementPhysics);
		if (bRunsMovementPhysics)
		{
			PhysicsMovement->SetAnimationStateOverride(
				false,
				FVector::ZeroVector,
				FVector::ZeroVector,
				true);
		}
		else
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
	}

	Super::Tick(DeltaSeconds);
	UpdateClientPhysicsStateReplication(DeltaSeconds);

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

	// 소유 클라이언트에서도 즉시 물리를 적용하고 동일 입력을 서버에 전달합니다.
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
		Super::ApplyJumpRequest();
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
		bLocalRightHandActive = bActive;
		// Grab 판정은 서버에 맡기고 손 표현만 로컬에서 즉시 갱신합니다.
		SetRightHandVisualState(bActive);
		if (!bActive)
		{
			RightHandGrab->ClearReplicatedGrab();
		}
		ServerSetRightHandActive(bActive);
	}
}

FRotator ANPRStablePhysicsPawn::GetTargetViewRotation() const
{
	if (IsLocallyControlled() && Controller)
	{
		return Controller->GetControlRotation();
	}

	return ReplicatedViewRotation;
}

void ANPRStablePhysicsPawn::ServerSetMoveInput_Implementation(
	FVector_NetQuantizeNormal WorldMoveInput)
{
	if (IsPhotoMovementLocked())
	{
		Super::ApplyMoveInput(FVector::ZeroVector);
		return;
	}
	Super::ApplyMoveInput(FVector(WorldMoveInput));
}

void ANPRStablePhysicsPawn::ServerSetViewRotation_Implementation(
	uint16 CompressedYaw,
	uint16 CompressedPitch)
{
	SetReplicatedViewRotation(FRotator(
		FRotator::DecompressAxisFromShort(CompressedPitch),
		FRotator::DecompressAxisFromShort(CompressedYaw),
		0.0f));
}

void ANPRStablePhysicsPawn::ServerStopMove_Implementation()
{
	Super::ApplyMoveInput(FVector::ZeroVector);
}

void ANPRStablePhysicsPawn::ServerRequestJump_Implementation()
{
	Super::ApplyJumpRequest();
}

void ANPRStablePhysicsPawn::ServerSetClientPhysicsState_Implementation(
	const FRigidBodyState& ClientState)
{
	if (ClientState.Position.ContainsNaN()
		|| ClientState.Quaternion.ContainsNaN()
		|| !ClientState.Quaternion.IsNormalized()
		|| ClientState.LinVel.ContainsNaN()
		|| ClientState.AngVel.ContainsNaN())
	{
		return;
	}

	FBodyInstance* PelvisBody = PhysicsMesh->GetBodyInstance(FullBodyRootName);
	if (!PelvisBody)
	{
		return;
	}

	PelvisBody->SetBodyTransform(
		FTransform(ClientState.Quaternion, FVector(ClientState.Position)),
		ETeleportType::TeleportPhysics);
	PelvisBody->SetLinearVelocity(FVector(ClientState.LinVel), false);
	PelvisBody->SetAngularVelocityInRadians(
		FMath::DegreesToRadians(FVector(ClientState.AngVel)),
		false);
}

void ANPRStablePhysicsPawn::ServerSetRightHandActive_Implementation(
	bool bActive)
{
	SetServerRightHandState(bActive);
}

void ANPRStablePhysicsPawn::OnRep_RightHandActive()
{
	SetRightHandVisualState(
		IsLocallyControlled()
			? bLocalRightHandActive
			: bReplicatedRightHandActive);
}

void ANPRStablePhysicsPawn::OnRep_GrabState()
{
	if (IsLocallyControlled() && !bLocalRightHandActive)
	{
		RightHandGrab->ClearReplicatedGrab();
		return;
	}

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
	if (IsLocallyControlled()
		&& RightHandGrab->GetGrabbedComponent() == GrabbedComponent)
	{
		return;
	}

	RightHandGrab->ApplyReplicatedGrab(
		GrabbedComponent,
		ReplicatedGrabState.GrabbedBoneName,
		ReplicatedGrabState.ConstraintFrame1,
		ReplicatedGrabState.ConstraintFrame2);
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

void ANPRStablePhysicsPawn::UpdateClientPhysicsStateReplication(float DeltaSeconds)
{
	if (HasAuthority() || !IsLocallyControlled())
	{
		return;
	}

	ClientPhysicsStateSendAccumulator += DeltaSeconds;
	constexpr float SendInterval = 1.0f / 30.0f;
	if (ClientPhysicsStateSendAccumulator < SendInterval)
	{
		return;
	}

	ClientPhysicsStateSendAccumulator -= SendInterval;
	FRigidBodyState ClientState;
	if (PhysicsMesh->GetRigidBodyState(ClientState, FullBodyRootName))
	{
		ServerSetClientPhysicsState(ClientState);
	}
}

void ANPRStablePhysicsPawn::UpdateViewRotationReplication(float DeltaSeconds)
{
	if (HasAuthority())
	{
		if (IsLocallyControlled())
		{
			SetReplicatedViewRotation(Super::GetTargetViewRotation());
		}
		return;
	}

	if (!IsLocallyControlled() || !Controller)
	{
		return;
	}

	ViewRotationSendAccumulator += DeltaSeconds;
	if (ViewRotationSendAccumulator < 0.05f)
	{
		return;
	}

	ViewRotationSendAccumulator -= 0.05f;
	const FRotator ViewRotation = Controller->GetControlRotation();
	ServerSetViewRotation(
		FRotator::CompressAxisToShort(ViewRotation.Yaw),
		FRotator::CompressAxisToShort(ViewRotation.Pitch));
}

void ANPRStablePhysicsPawn::SetReplicatedViewRotation(
	const FRotator& NewViewRotation)
{
	ReplicatedViewRotation = FRotator(
		FMath::Clamp(
			FRotator::NormalizeAxis(NewViewRotation.Pitch),
			-89.9f,
			89.9f),
		FRotator::NormalizeAxis(NewViewRotation.Yaw),
		0.0f);
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
