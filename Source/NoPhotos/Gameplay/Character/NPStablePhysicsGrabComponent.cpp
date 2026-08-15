#include "Gameplay/Character/NPStablePhysicsGrabComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Gameplay/Interaction/Components/GrabbableComponent.h"

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

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SetLinearBreakable(true, GrabLinearBreakThreshold);
		OnConstraintBroken.AddDynamic(
			this,
			&UNPStablePhysicsGrabComponent::HandleConstraintBroken);
	}
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
		bWaitForGrabRelease = false;
		ReleaseGrab();
	}
}

void UNPStablePhysicsGrabComponent::SetGrabSimulationEnabled(bool bEnabled)
{
	bGrabSimulationEnabled = bEnabled;
	if (!bGrabSimulationEnabled)
	{
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
	if (!IsValid(PrimitiveComponent) || !PhysicsMesh)
	{
		ClearReplicatedGrab();
		return;
	}

	if (IsHoldingObject())
	{
		ReleaseGrab();
	}

	GrabbedComponent = PrimitiveComponent;
	GrabbedBoneName = BoneName;
	SetConstrainedComponents(
		PhysicsMesh,
		HandBoneName,
		GrabbedComponent,
		GrabbedBoneName);
	SetConstraintReferenceFrame(EConstraintFrame::Frame1, Frame1);
	SetConstraintReferenceFrame(EConstraintFrame::Frame2, Frame2);
	OnGrabbedComponentChanged.Broadcast(GrabbedComponent);
}

void UNPStablePhysicsGrabComponent::ClearReplicatedGrab()
{
	if (IsHoldingObject())
	{
		ReleaseGrab();
		return;
	}

	BreakConstraint();
	GrabbedComponent = nullptr;
	GrabbedBoneName = NAME_None;
}

void UNPStablePhysicsGrabComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bGrabSimulationEnabled
		&& bGrabRequested
		&& !bWaitForGrabRelease
		&& !IsHoldingObject())
	{
		TryGrab();
	}

	DrawGrabDebug();
}

void UNPStablePhysicsGrabComponent::HandleConstraintBroken(int32)
{
	if (!IsHoldingObject())
	{
		return;
	}

	bWaitForGrabRelease = true;
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

		GrabbableComponent->NotifyGrabStarted(PrimitiveComponent);
		if (!PrimitiveComponent->IsSimulatingPhysics())
		{
			continue;
		}

		Grab(PrimitiveComponent);
		return;
	}
}

void UNPStablePhysicsGrabComponent::Grab(UPrimitiveComponent* PrimitiveComponent)
{
	GrabbedComponent = PrimitiveComponent;
	GrabbedBoneName = NAME_None;
	SetWorldLocation(PhysicsMesh->GetSocketLocation(HandBoneName));

	// Constraint를 통해 손 Body와 물체가 서로 물리적인 힘을 주고받습니다.
	SetConstrainedComponents(
		PhysicsMesh,
		HandBoneName,
		GrabbedComponent,
		NAME_None);
	OnGrabbedComponentChanged.Broadcast(GrabbedComponent);
}

void UNPStablePhysicsGrabComponent::ReleaseGrab()
{
	if (!IsHoldingObject())
	{
		return;
	}

	BreakConstraint();
	GrabbedComponent = nullptr;
	GrabbedBoneName = NAME_None;
	OnGrabbedComponentChanged.Broadcast(nullptr);
}

void UNPStablePhysicsGrabComponent::DrawGrabDebug() const
{
	if (!bDrawGrabDebug
		|| !PhysicsMesh
		|| !GetWorld()
		|| PhysicsMesh->GetBoneIndex(HandBoneName) == INDEX_NONE)
	{
		return;
	}

	const FVector HandLocation = PhysicsMesh->GetSocketLocation(HandBoneName);
	DrawDebugSphere(
		GetWorld(),
		HandLocation,
		GrabRadius,
		16,
		IsHoldingObject() ? FColor::Green : FColor::Yellow,
		false,
		0.0f,
		0,
		2.0f);

	if (IsHoldingObject())
	{
		DrawDebugLine(
			GetWorld(),
			HandLocation,
			GrabbedComponent->GetComponentLocation(),
			FColor::Green,
			false,
			0.0f,
			0,
			3.0f);
	}
}
