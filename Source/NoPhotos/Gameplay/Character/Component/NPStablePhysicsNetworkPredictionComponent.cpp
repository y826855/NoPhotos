#include "Gameplay/Character/Component/NPStablePhysicsNetworkPredictionComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Gameplay/Character/Component/NPStablePhysicsGrabComponent.h"
#include "Gameplay/Character/Component/NPStablePhysicsMovementComponent.h"

UNPStablePhysicsNetworkPredictionComponent::UNPStablePhysicsNetworkPredictionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	SetIsReplicatedByDefault(true);
}

void UNPStablePhysicsNetworkPredictionComponent::Initialize(
	USkeletalMeshComponent* InPhysicsMesh,
	UNPStablePhysicsMovementComponent* InMovement,
	UNPStablePhysicsGrabComponent* InGrab,
	FName InRootBodyName)
{
	PhysicsMesh = InPhysicsMesh;
	Movement = InMovement;
	Grab = InGrab;
	RootBodyName = InRootBodyName;

	if (Movement)
	{
		AddTickPrerequisiteComponent(Movement);
	}
	if (Grab)
	{
		AddTickPrerequisiteComponent(Grab);
	}
}

void UNPStablePhysicsNetworkPredictionComponent::SendMoveInput(
	const FVector& WorldMoveInput)
{
	if (!GetOwner() || GetOwner()->HasAuthority())
	{
		return;
	}

	PendingMoveInput = WorldMoveInput.GetClampedToMaxSize(1.0f);
}

void UNPStablePhysicsNetworkPredictionComponent::SendStopMove()
{
	if (!GetOwner() || GetOwner()->HasAuthority())
	{
		return;
	}

	++LocalInputSequence;
	PendingMoveInput = FVector::ZeroVector;
	InputSendAccumulator = 0.0f;
	ServerStopMove(LocalInputSequence);
}

void UNPStablePhysicsNetworkPredictionComponent::SendJumpRequest()
{
	if (!GetOwner() || GetOwner()->HasAuthority())
	{
		return;
	}

	ServerRequestJump();
}

void UNPStablePhysicsNetworkPredictionComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(
		UNPStablePhysicsNetworkPredictionComponent,
		ServerRootState,
		COND_OwnerOnly);
}

void UNPStablePhysicsNetworkPredictionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !PhysicsMesh || RootBodyName.IsNone())
	{
		return;
	}

	if (GetOwner()->HasAuthority())
	{
		CaptureServerRootState();
		return;
	}

	if (GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		SendPendingMoveInput(DeltaTime);
		ApplyOwnerCorrection(DeltaTime);
	}
}

void UNPStablePhysicsNetworkPredictionComponent::ServerSetMoveInput_Implementation(
	uint16 InputSequence,
	FVector_NetQuantizeNormal WorldMoveInput)
{
	if (!Movement)
	{
		return;
	}

	const FVector ReceivedInput = FVector(WorldMoveInput);
	if (ReceivedInput.ContainsNaN() || !AcceptInputSequence(InputSequence))
	{
		return;
	}

	const FVector ClampedInput = ReceivedInput.GetClampedToMaxSize(1.0f);
	Movement->SetMoveInput(ClampedInput);
	if (Grab)
	{
		Grab->SetMovementIntent(ClampedInput);
	}
}

void UNPStablePhysicsNetworkPredictionComponent::ServerStopMove_Implementation(
	uint16 InputSequence)
{
	if (!Movement || !AcceptInputSequence(InputSequence))
	{
		return;
	}

	Movement->SetMoveInput(FVector::ZeroVector);
	if (Grab)
	{
		Grab->SetMovementIntent(FVector::ZeroVector);
	}
}

void UNPStablePhysicsNetworkPredictionComponent::ServerRequestJump_Implementation()
{
	if (!Movement)
	{
		return;
	}

	Movement->RequestJump();
}

void UNPStablePhysicsNetworkPredictionComponent::OnRep_ServerRootState()
{
	bHasServerRootState = true;
}

void UNPStablePhysicsNetworkPredictionComponent::CaptureServerRootState()
{
	FBodyInstance* RootBody = PhysicsMesh->GetBodyInstance(RootBodyName);
	if (!RootBody)
	{
		return;
	}

	const FTransform RootTransform = RootBody->GetUnrealWorldTransform();
	ServerRootState.Position = RootTransform.GetLocation();
	ServerRootState.Rotation = RootTransform.GetRotation();
	ServerRootState.LinearVelocity = RootBody->GetUnrealWorldVelocity();
	ServerRootState.AngularVelocity = FMath::RadiansToDegrees(
		RootBody->GetUnrealWorldAngularVelocityInRadians());
	ServerRootState.ServerWorldTime = GetWorld()->GetTimeSeconds();
}

void UNPStablePhysicsNetworkPredictionComponent::ApplyOwnerCorrection(
	float DeltaTime)
{
	if (!bHasServerRootState || DeltaTime <= UE_SMALL_NUMBER)
	{
		return;
	}

	FBodyInstance* RootBody = PhysicsMesh->GetBodyInstance(RootBodyName);
	if (!RootBody)
	{
		return;
	}

	const float ServerStateAge =
		GetEstimatedServerWorldTime() - ServerRootState.ServerWorldTime;
	if (ServerStateAge > MaximumServerStateAge)
	{
		return;
	}

	const float ExtrapolationTime = FMath::Clamp(
		ServerStateAge,
		0.0f,
		MaximumExtrapolationTime);
	const FVector ExpectedServerPosition = FVector(ServerRootState.Position)
		+ FVector(ServerRootState.LinearVelocity) * ExtrapolationTime;
	const FVector CurrentRootPosition =
		RootBody->GetUnrealWorldTransform().GetLocation();
	const FVector PositionError =
		ExpectedServerPosition - CurrentRootPosition;
	const bool bNearUnheldDynamicBody =
		IsNearUnheldDynamicBody(CurrentRootPosition);

	if (!bNearUnheldDynamicBody
		&& PositionError.SizeSquared() > FMath::Square(HardSnapDistance))
	{
		RootBody->SetBodyTransform(
			FTransform(ServerRootState.Rotation, ExpectedServerPosition),
			ETeleportType::TeleportPhysics);
		RootBody->SetLinearVelocity(FVector(ServerRootState.LinearVelocity), false);
		RootBody->SetAngularVelocityInRadians(
			FMath::DegreesToRadians(FVector(ServerRootState.AngularVelocity)),
			false);
		return;
	}

	if (PositionError.SizeSquared() <= FMath::Square(PositionErrorTolerance))
	{
		return;
	}

	const float SafeCorrectionTime = FMath::Max(PositionCorrectionTime, 0.01f);
	const FVector CorrectionVelocity = (PositionError / SafeCorrectionTime)
		.GetClampedToMaxSize(MaximumCorrectionSpeed);
	const FVector DesiredVelocity =
		FVector(ServerRootState.LinearVelocity) + CorrectionVelocity;
	FVector CorrectionAcceleration =
		(DesiredVelocity - RootBody->GetUnrealWorldVelocity())
		/ SafeCorrectionTime;
	CorrectionAcceleration = CorrectionAcceleration.GetClampedToMaxSize(
		MaximumCorrectionAcceleration);
	if (bNearUnheldDynamicBody)
	{
		CorrectionAcceleration *= DynamicBodyCorrectionScale;
	}

	PhysicsMesh->AddForce(CorrectionAcceleration, RootBodyName, true);
}

bool UNPStablePhysicsNetworkPredictionComponent::IsNearUnheldDynamicBody(
	const FVector& RootPosition) const
{
	if (!GetWorld() || DynamicBodyDetectionRadius <= UE_SMALL_NUMBER)
	{
		return false;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(StablePhysicsNetworkCorrection),
		false,
		GetOwner());
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		RootPosition,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(DynamicBodyDetectionRadius),
		QueryParams);

	const UPrimitiveComponent* GrabbedComponent =
		Grab ? Grab->GetGrabbedComponent() : nullptr;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const UPrimitiveComponent* OverlappedComponent = Overlap.GetComponent();
		if (OverlappedComponent
			&& OverlappedComponent != GrabbedComponent
			&& OverlappedComponent->IsSimulatingPhysics())
		{
			return true;
		}
	}

	return false;
}

void UNPStablePhysicsNetworkPredictionComponent::SendPendingMoveInput(
	float DeltaTime)
{
	if (PendingMoveInput.IsNearlyZero())
	{
		return;
	}

	InputSendAccumulator += DeltaTime;
	const float SafeSendInterval = FMath::Max(InputSendInterval, 0.01f);
	if (InputSendAccumulator < SafeSendInterval)
	{
		return;
	}

	InputSendAccumulator = FMath::Fmod(InputSendAccumulator, SafeSendInterval);
	++LocalInputSequence;
	ServerSetMoveInput(LocalInputSequence, PendingMoveInput);
}

bool UNPStablePhysicsNetworkPredictionComponent::AcceptInputSequence(
	uint16 InputSequence)
{
	if (bHasReceivedInput
		&& static_cast<int16>(InputSequence - LastServerInputSequence) <= 0)
	{
		return false;
	}

	bHasReceivedInput = true;
	LastServerInputSequence = InputSequence;
	return true;
}

float UNPStablePhysicsNetworkPredictionComponent::GetEstimatedServerWorldTime() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const AGameStateBase* GameState = World->GetGameState())
		{
			return GameState->GetServerWorldTimeSeconds();
		}
		return World->GetTimeSeconds();
	}

	return ServerRootState.ServerWorldTime;
}
