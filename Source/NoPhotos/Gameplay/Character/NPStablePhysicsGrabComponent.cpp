#include "Gameplay/Character/NPStablePhysicsGrabComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Gameplay/Character/NPStablePhysicsPawn.h"
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
		ReleaseGrab();
		ClearCandidate();
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

bool UNPStablePhysicsGrabComponent::GetVisualGrabPoint(FVector& OutWorldPoint) const
{
	if (IsHoldingObject())
	{
		OutWorldPoint = GrabbedComponent->GetComponentTransform().TransformPosition(GrabPointLocal);
		return true;
	}
	if (HasGrabCandidate())
	{
		OutWorldPoint = CandidateGrabPoint;
		return true;
	}
	return false;
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
	const FTransform& Frame2,
	const FVector& InGrabPointLocal)
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
	GrabPointLocal = InGrabPointLocal;
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
	GrabPointLocal = FVector::ZeroVector;
}

void UNPStablePhysicsGrabComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bGrabRequested && !IsHoldingObject())
	{
		if (!HasGrabCandidate())
		{
			UpdateCandidate();
		}
		else
		{
			CandidateGrabPoint = CandidateComponent->GetComponentTransform()
				.TransformPosition(CandidateGrabPointLocal);
		}
		if (bGrabSimulationEnabled && HasGrabCandidate() && PhysicsMesh
			&& PhysicsMesh->GetBoneIndex(HandBoneName) != INDEX_NONE
			&& FVector::DistSquared(
				PhysicsMesh->GetSocketLocation(HandBoneName),
				CandidateGrabPoint) <= FMath::Square(GrabAttachDistance))
		{
			Grab(CandidateComponent, CandidateBoneName, CandidateGrabPoint);
		}
	}
	else if (!bGrabRequested)
	{
		ClearCandidate();
	}

	DrawGrabDebug();
}

void UNPStablePhysicsGrabComponent::UpdateCandidate()
{
	const ANPStablePhysicsPawn* PawnOwner = Cast<ANPStablePhysicsPawn>(GetOwner());
	if (!GetWorld() || !PhysicsMesh || !PawnOwner)
	{
		ClearCandidate();
		return;
	}

	FVector SearchForward = PawnOwner->GetVisualForwardDirection();
	SearchForward.Z = 0.0f;
	SearchForward = SearchForward.GetSafeNormal();
	if (SearchForward.IsNearlyZero())
	{
		ClearCandidate();
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(StablePhysicsGrab), false, GetOwner());
	const float SearchHalfDepth = FrontSearchDistance * 0.5f;
	const FVector SearchOrigin = PhysicsMesh->GetComponentLocation()
		+ FVector::UpVector * FrontSearchVerticalOffset;
	const FVector SearchCenter = SearchOrigin + SearchForward * SearchHalfDepth;
	const FQuat SearchRotation = SearchForward.Rotation().Quaternion();
	const FVector SearchExtent(
		SearchHalfDepth,
		FrontSearchHalfWidth,
		FrontSearchHalfHeight);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		SearchCenter,
		SearchRotation,
		ObjectQueryParams,
		FCollisionShape::MakeBox(SearchExtent),
		QueryParams);

	const FVector HandLocation = PhysicsMesh->GetSocketLocation(HandBoneName);
	float BestDistanceSquared = TNumericLimits<float>::Max();
	UPrimitiveComponent* BestComponent = nullptr;
	FVector BestPoint = FVector::ZeroVector;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		UPrimitiveComponent* PrimitiveComponent = Overlap.GetComponent();
		if (!PrimitiveComponent)
		{
			continue;
		}

		FVector ClosestPoint;
		if (PrimitiveComponent->GetClosestPointOnCollision(
			HandLocation, ClosestPoint, NAME_None) < 0.0f)
		{
			ClosestPoint = PrimitiveComponent->Bounds.Origin;
		}

		if (!IsValidCandidate(PrimitiveComponent, NAME_None))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(HandLocation, ClosestPoint);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestComponent = PrimitiveComponent;
			BestPoint = ClosestPoint;
		}
	}

	if (!BestComponent)
	{
		ClearCandidate();
		return;
	}

	CandidateComponent = BestComponent;
	CandidateBoneName = NAME_None;
	CandidateGrabPoint = BestPoint;
	CandidateGrabPointLocal = CandidateComponent->GetComponentTransform()
		.InverseTransformPosition(CandidateGrabPoint);
}

bool UNPStablePhysicsGrabComponent::IsValidCandidate(
	UPrimitiveComponent* PrimitiveComponent,
	FName BoneName) const
{
	AActor* OwnerActor = PrimitiveComponent ? PrimitiveComponent->GetOwner() : nullptr;
	if (!PrimitiveComponent || !PrimitiveComponent->IsSimulatingPhysics(BoneName)
		|| !OwnerActor || !OwnerActor->FindComponentByClass<UGrabbableComponent>())
	{
		return false;
	}

	if (MaximumGrabMass > 0.0f && PrimitiveComponent->GetMass() > MaximumGrabMass)
	{
		return false;
	}

	FHitResult SightHit;
	FCollisionQueryParams SightParams(SCENE_QUERY_STAT(StablePhysicsGrabSight), true, GetOwner());
	// 바닥에 닿은 작은 물체의 표면점은 바닥과 겹칠 수 있으므로 Bounds 중심으로 가시성을 검사합니다.
	const FVector SightTarget = PrimitiveComponent->Bounds.Origin;
	const bool bSightBlocked = GetWorld()->LineTraceSingleByChannel(
		SightHit,
		PhysicsMesh->GetSocketLocation(HandBoneName),
		SightTarget,
		ECC_Visibility,
		SightParams);
	if (bSightBlocked && SightHit.GetComponent() != PrimitiveComponent)
	{
		return false;
	}

	return true;
}

void UNPStablePhysicsGrabComponent::ClearCandidate()
{
	CandidateComponent = nullptr;
	CandidateBoneName = NAME_None;
	CandidateGrabPoint = FVector::ZeroVector;
	CandidateGrabPointLocal = FVector::ZeroVector;
}

void UNPStablePhysicsGrabComponent::Grab(
	UPrimitiveComponent* PrimitiveComponent,
	FName BoneName,
	const FVector& WorldGrabPoint)
{
	GrabbedComponent = PrimitiveComponent;
	GrabbedBoneName = BoneName;
	GrabPointLocal = GrabbedComponent->GetComponentTransform().InverseTransformPosition(WorldGrabPoint);
	SetWorldLocation(WorldGrabPoint);

	// Constraint를 통해 손 Body와 물체가 서로 물리적인 힘을 주고받습니다.
	SetConstrainedComponents(
		PhysicsMesh,
		HandBoneName,
		GrabbedComponent,
		GrabbedBoneName);
	ClearCandidate();
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
	GrabPointLocal = FVector::ZeroVector;
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
	const ANPStablePhysicsPawn* PawnOwner = Cast<ANPStablePhysicsPawn>(GetOwner());
	if (PawnOwner)
	{
		FVector SearchForward = PawnOwner->GetVisualForwardDirection();
		SearchForward.Z = 0.0f;
		SearchForward = SearchForward.GetSafeNormal();
		const float SearchHalfDepth = FrontSearchDistance * 0.5f;
		const FVector SearchCenter = PhysicsMesh->GetComponentLocation()
			+ FVector::UpVector * FrontSearchVerticalOffset
			+ SearchForward * SearchHalfDepth;
		DrawDebugBox(
			GetWorld(),
			SearchCenter,
			FVector(SearchHalfDepth, FrontSearchHalfWidth, FrontSearchHalfHeight),
			SearchForward.Rotation().Quaternion(),
			FColor::Blue,
			false,
			0.0f,
			0,
			1.0f);
	}
	DrawDebugSphere(
		GetWorld(),
		HandLocation,
		GrabAttachDistance,
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
			GrabbedComponent->GetComponentTransform().TransformPosition(GrabPointLocal),
			FColor::Green,
			false,
			0.0f,
			0,
			3.0f);
	}
	else if (HasGrabCandidate())
	{
		DrawDebugSphere(GetWorld(), CandidateGrabPoint, 7.0f, 12, FColor::Cyan, false, 0.0f, 0, 2.0f);
		DrawDebugLine(GetWorld(), HandLocation, CandidateGrabPoint, FColor::Cyan, false, 0.0f, 0, 2.0f);
	}
}
