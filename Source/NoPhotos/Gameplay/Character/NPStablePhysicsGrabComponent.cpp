#include "Gameplay/Character/NPStablePhysicsGrabComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Gameplay/Character/NPStablePhysicsDebugComponent.h"
#include "Gameplay/Interaction/Components/GrabbableComponent.h"
#include "PhysicsEngine/BodyInstance.h"

UNPStablePhysicsGrabComponent::UNPStablePhysicsGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	SetDisableCollision(true);
	SetLinearXLimit(LCM_Locked, 0.0f);
	SetLinearYLimit(LCM_Locked, 0.0f);
	SetLinearZLimit(LCM_Locked, 0.0f);
	SetAngularSwing1Limit(ACM_Limited, 45.0f);
	SetAngularSwing2Limit(ACM_Limited, 45.0f);
	SetAngularTwistLimit(ACM_Limited, 45.0f);
}

void UNPStablePhysicsGrabComponent::BeginPlay()
{
	Super::BeginPlay();
	PhysicsDebug = GetOwner()
		? GetOwner()->FindComponentByClass<UNPStablePhysicsDebugComponent>()
		: nullptr;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SetLinearBreakable(true, GrabLinearBreakThreshold);
		OnConstraintBroken.AddDynamic(
			this,
			&UNPStablePhysicsGrabComponent::HandleConstraintBroken);
	}
}

void UNPStablePhysicsGrabComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	ClearReplicatedGrab();
	Super::EndPlay(EndPlayReason);
}

void UNPStablePhysicsGrabComponent::Initialize(
	USkeletalMeshComponent* InPhysicsMesh,
	FName InHandBoneName)
{
	PhysicsMesh = InPhysicsMesh;
	HandBoneName = InHandBoneName;
}

void UNPStablePhysicsGrabComponent::SetGrabRequested(bool bRequested)
{
	bGrabRequested = bRequested;
	if (!bGrabRequested)
	{
		bGrabRetryCoolingDown = false;
		GrabRetryCooldownRemaining = 0.0f;
		ReleaseGrab();
	}
}

void UNPStablePhysicsGrabComponent::SetGrabSimulationEnabled(bool bEnabled)
{
	bGrabSimulationEnabled = bEnabled;
	if (!bGrabSimulationEnabled)
	{
		bGrabRetryCoolingDown = false;
		GrabRetryCooldownRemaining = 0.0f;
		ReleaseGrab();
	}
}

void UNPStablePhysicsGrabComponent::SetLinearBreakThreshold(
	float InLinearBreakThreshold)
{
	GrabLinearBreakThreshold = FMath::Max(InLinearBreakThreshold, 0.0f);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SetLinearBreakable(true, GrabLinearBreakThreshold);
	}
}

void UNPStablePhysicsGrabComponent::SetReplicatedGrabFrameBlendDuration(
	float InBlendDuration)
{
	ReplicatedGrabFrameBlendDuration = FMath::Max(InBlendDuration, 0.0f);
}

void UNPStablePhysicsGrabComponent::SetGrabRetryCooldown(
	float InRetryCooldown)
{
	GrabRetryCooldown = FMath::Max(InRetryCooldown, 0.0f);
}

void UNPStablePhysicsGrabComponent::SetMovementIntent(
	const FVector& WorldMovementIntent)
{
	MovementIntent = WorldMovementIntent.GetClampedToMaxSize(1.0f);
}

void UNPStablePhysicsGrabComponent::NotifyJumpIntent()
{
	JumpIntentRemainingTime = JumpIntentDuration;
}

FTransform UNPStablePhysicsGrabComponent::GetGrabConstraintFrame(
	EConstraintFrame::Type Frame) const
{
	return ConstraintInstance.GetRefFrame(Frame);
}

void UNPStablePhysicsGrabComponent::ApplyReplicatedGrab(
	UPrimitiveComponent* PrimitiveComponent,
	FName BoneName,
	const FTransform& Frame1,
	const FTransform& Frame2)
{
	bReplicatedGrabFrameBlendActive = false;
	if (!IsValid(PrimitiveComponent) || !PhysicsMesh)
	{
		ClearReplicatedGrab();
		return;
	}

	if (IsHoldingObject())
	{
		ReleaseGrab();
	}

	UGrabbableComponent* GrabbableComponent = PrimitiveComponent->GetOwner()
		? PrimitiveComponent->GetOwner()->FindComponentByClass<UGrabbableComponent>()
		: nullptr;

	FTransform InitialFrame1 = Frame1;
	FBodyInstance* HandBody = PhysicsMesh->GetBodyInstance(HandBoneName);
	FBodyInstance* TargetBody = PrimitiveComponent->GetBodyInstance(BoneName);
	bool bShouldBlendFrame = false;
	if (HandBody && TargetBody)
	{
		const FTransform TargetWorldFrame =
			Frame2 * TargetBody->GetUnrealWorldTransform();
		InitialFrame1 = TargetWorldFrame.GetRelativeTransform(
			HandBody->GetUnrealWorldTransform());

		bShouldBlendFrame =
			ReplicatedGrabFrameBlendDuration > UE_SMALL_NUMBER
			&& !InitialFrame1.Equals(Frame1);
	}

	SetConstrainedComponents(
		PhysicsMesh,
		HandBoneName,
		PrimitiveComponent,
		BoneName);
	SetConstraintReferenceFrame(EConstraintFrame::Frame1, InitialFrame1);
	SetConstraintReferenceFrame(EConstraintFrame::Frame2, Frame2);
	if (!CommitGrab(PrimitiveComponent, GrabbableComponent, BoneName))
	{
		return;
	}

	ReplicatedGrabFrameBlendStart = InitialFrame1;
	ReplicatedGrabFrameBlendTarget = Frame1;
	ReplicatedGrabFrameBlendElapsed = 0.0f;
	bReplicatedGrabFrameBlendActive = bShouldBlendFrame;
}

void UNPStablePhysicsGrabComponent::ClearReplicatedGrab()
{
	bReplicatedGrabFrameBlendActive = false;
	if (IsHoldingObject())
	{
		ReleaseGrab();
		return;
	}

	UGrabbableComponent* ReleasedGrabbableComponent =
		GrabbedGrabbableComponent;
	if (ReleasedGrabbableComponent)
	{
		ReleasedGrabbableComponent->OnForceReleaseAllGrabs.RemoveAll(this);
	}
	BreakConstraint();
	GrabbedComponent = nullptr;
	GrabbedGrabbableComponent = nullptr;
	GrabbedBoneName = NAME_None;
	if (ReleasedGrabbableComponent)
	{
		ReleasedGrabbableComponent->NotifyGrabEnded();
	}
}

void UNPStablePhysicsGrabComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bGrabRetryCoolingDown && bGrabRequested)
	{
		GrabRetryCooldownRemaining = FMath::Max(
			GrabRetryCooldownRemaining - DeltaTime,
			0.0f);
		if (GrabRetryCooldownRemaining <= 0.0f)
		{
			bGrabRetryCoolingDown = false;
		}
	}

	if (bGrabSimulationEnabled
		&& bGrabRequested
		&& !bGrabRetryCoolingDown
		&& !IsHoldingObject())
	{
		TryGrab();
	}

	if (bGrabSimulationEnabled && IsHoldingObject())
	{
		UpdateGrabForce(DeltaTime);
	}
	if (bReplicatedGrabFrameBlendActive && IsHoldingObject())
	{
		UpdateReplicatedGrabFrameBlend(DeltaTime);
	}
	JumpIntentRemainingTime = FMath::Max(
		JumpIntentRemainingTime - DeltaTime,
		0.0f);

}

void UNPStablePhysicsGrabComponent::HandleConstraintBroken(int32)
{
	if (!IsHoldingObject())
	{
		return;
	}

	bGrabRetryCoolingDown = true;
	GrabRetryCooldownRemaining = GrabRetryCooldown;
	ReleaseGrab();
}

void UNPStablePhysicsGrabComponent::HandleForceReleaseAllGrabs()
{
	bGrabRetryCoolingDown = true;
	GrabRetryCooldownRemaining = GrabRetryCooldown;
	ReleaseGrab();
}

void UNPStablePhysicsGrabComponent::TryGrab()
{
	if (!PhysicsMesh
		|| !GetWorld()
		|| PhysicsMesh->GetBoneIndex(HandBoneName) == INDEX_NONE)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(StablePhysicsGrab), false, GetOwner());
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		PhysicsMesh->GetSocketLocation(HandBoneName),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(GrabRadius),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		UPrimitiveComponent* PrimitiveComponent = Overlap.GetComponent();
		AActor* OwnerActor = PrimitiveComponent ? PrimitiveComponent->GetOwner() : nullptr;

		if (!PrimitiveComponent || !OwnerActor)
		{
			continue;
		}

		UGrabbableComponent* GrabbableComponent =
			OwnerActor->FindComponentByClass<UGrabbableComponent>();
		if (!GrabbableComponent || !GrabbableComponent->CanBeGrabbed())
		{
			continue;
		}

		UPrimitiveComponent* GrabTarget =
			GrabbableComponent->ResolveGrabTarget(PrimitiveComponent);
		if (GrabTarget && Grab(GrabTarget, GrabbableComponent))
		{
			return;
		}
	}
}

bool UNPStablePhysicsGrabComponent::Grab(
	UPrimitiveComponent* PrimitiveComponent,
	UGrabbableComponent* GrabbableComponent)
{
	SetWorldLocation(PhysicsMesh->GetSocketLocation(HandBoneName));

	// Constraint를 통해 손 Body와 물체가 서로 물리적인 힘을 주고받습니다.
	SetConstrainedComponents(
		PhysicsMesh,
		HandBoneName,
		PrimitiveComponent,
		NAME_None);
	return CommitGrab(PrimitiveComponent, GrabbableComponent, NAME_None);
}

bool UNPStablePhysicsGrabComponent::CommitGrab(
	UPrimitiveComponent* PrimitiveComponent,
	UGrabbableComponent* GrabbableComponent,
	FName BoneName)
{
	if (!GrabbableComponent
		|| !GrabbableComponent->CanBeGrabbed()
		|| !ConstraintInstance.IsValidConstraintInstance())
	{
		BreakConstraint();
		return false;
	}

	GrabbedComponent = PrimitiveComponent;
	GrabbedGrabbableComponent = GrabbableComponent;
	GrabbedBoneName = BoneName;
	GrabbedGrabbableComponent->OnForceReleaseAllGrabs.AddUObject(
		this,
		&UNPStablePhysicsGrabComponent::HandleForceReleaseAllGrabs);
	if (PhysicsDebug)
	{
		PhysicsDebug->ResetGrabDebug();
	}
	if (GrabbedGrabbableComponent)
	{
		GrabbedGrabbableComponent->NotifyGrabStarted(GrabbedComponent);
	}
	OnGrabbedComponentChanged.Broadcast(GrabbedComponent);
	return true;
}

void UNPStablePhysicsGrabComponent::UpdateGrabForce(float DeltaTime)
{
	if (!GrabbedGrabbableComponent
		|| !ConstraintInstance.IsValidConstraintInstance()
		|| DeltaTime <= UE_SMALL_NUMBER)
	{
		return;
	}

	FVector LinearImpulse = FVector::ZeroVector;
	FVector AngularImpulse = FVector::ZeroVector;
	GetConstraintForce(LinearImpulse, AngularImpulse);
	const FVector LinearForce = LinearImpulse / DeltaTime;
	const FVector RelicForce = -LinearForce;

	FVector UserIntent = MovementIntent;
	if (JumpIntentRemainingTime > 0.0f)
	{
		UserIntent += FVector::UpVector;
	}
	UserIntent = UserIntent.GetClampedToMaxSize(1.0f);

	FVector IntentAlignedForce = FVector::ZeroVector;
	float IntentForceAlignment = 0.0f;
	const FVector UserIntentDirection = UserIntent.GetSafeNormal();
	if (!UserIntentDirection.IsNearlyZero() && !RelicForce.IsNearlyZero())
	{
		IntentForceAlignment = FVector::DotProduct(
			RelicForce.GetSafeNormal(),
			UserIntentDirection);
		const float IntentForceMagnitude = FMath::Max(
			FVector::DotProduct(
				RelicForce,
				UserIntentDirection),
			0.0f);
		IntentAlignedForce = UserIntentDirection * IntentForceMagnitude;
	}
	if (PhysicsDebug)
	{
		PhysicsDebug->UpdateGrabDebug(
			*this,
			DeltaTime,
			RelicForce,
			UserIntent);
	}

	// Chaos 출력은 Constraint impulse이므로 DeltaTime으로 나눠 force로 변환합니다.
	GrabbedGrabbableComponent->NotifyGrabForce(
		IntentAlignedForce,
		-AngularImpulse / DeltaTime,
		IntentForceAlignment);
}

void UNPStablePhysicsGrabComponent::UpdateReplicatedGrabFrameBlend(
	float DeltaTime)
{
	ReplicatedGrabFrameBlendElapsed += DeltaTime;
	const float BlendAlpha = FMath::Clamp(
		ReplicatedGrabFrameBlendElapsed / ReplicatedGrabFrameBlendDuration,
		0.0f,
		1.0f);

	FTransform BlendedFrame;
	BlendedFrame.Blend(
		ReplicatedGrabFrameBlendStart,
		ReplicatedGrabFrameBlendTarget,
		BlendAlpha);
	SetConstraintReferenceFrame(EConstraintFrame::Frame1, BlendedFrame);

	if (BlendAlpha >= 1.0f)
	{
		bReplicatedGrabFrameBlendActive = false;
	}
}

void UNPStablePhysicsGrabComponent::ReleaseGrab()
{
	bReplicatedGrabFrameBlendActive = false;
	if (!IsHoldingObject())
	{
		return;
	}

	UGrabbableComponent* ReleasedGrabbableComponent =
		GrabbedGrabbableComponent;
	if (ReleasedGrabbableComponent)
	{
		ReleasedGrabbableComponent->OnForceReleaseAllGrabs.RemoveAll(this);
	}
	GrabbedComponent = nullptr;
	GrabbedGrabbableComponent = nullptr;
	GrabbedBoneName = NAME_None;
	BreakConstraint();
	if (ReleasedGrabbableComponent)
	{
		ReleasedGrabbableComponent->NotifyGrabEnded();
	}
	OnGrabbedComponentChanged.Broadcast(nullptr);
}
