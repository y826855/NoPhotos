#include "Gameplay/Character/NPStablePhysicsGrabComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
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
	ReleaseGrab();
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
		bWaitForGrabRelease = false;
		GrabRetryCooldownRemaining = 0.0f;
		ReleaseGrab();
	}
}

void UNPStablePhysicsGrabComponent::SetGrabSimulationEnabled(bool bEnabled)
{
	bGrabSimulationEnabled = bEnabled;
	if (!bGrabSimulationEnabled)
	{
		bWaitForGrabRelease = false;
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

	GrabbedComponent = PrimitiveComponent;
	GrabbedGrabbableComponent = PrimitiveComponent->GetOwner()
		? PrimitiveComponent->GetOwner()->FindComponentByClass<UGrabbableComponent>()
		: nullptr;
	GrabbedBoneName = BoneName;
	// 디버그 전용: 복제된 Grab 대상의 디버그 누적 상태를 초기화합니다.
	ResetGrabDebug();

	if (GrabbedGrabbableComponent)
	{
		GrabbedGrabbableComponent->NotifyGrabStarted(GrabbedComponent);
	}

	FTransform InitialFrame1 = Frame1;
	FBodyInstance* HandBody = PhysicsMesh->GetBodyInstance(HandBoneName);
	FBodyInstance* TargetBody = GrabbedComponent->GetBodyInstance(GrabbedBoneName);
	if (HandBody && TargetBody)
	{
		const FTransform TargetWorldFrame =
			Frame2 * TargetBody->GetUnrealWorldTransform();
		InitialFrame1 = TargetWorldFrame.GetRelativeTransform(
			HandBody->GetUnrealWorldTransform());

		ReplicatedGrabFrameBlendStart = InitialFrame1;
		ReplicatedGrabFrameBlendTarget = Frame1;
		ReplicatedGrabFrameBlendElapsed = 0.0f;
		bReplicatedGrabFrameBlendActive =
			ReplicatedGrabFrameBlendDuration > UE_SMALL_NUMBER
			&& !InitialFrame1.Equals(Frame1);
	}

	SetConstrainedComponents(
		PhysicsMesh,
		HandBoneName,
		GrabbedComponent,
		GrabbedBoneName);
	SetConstraintReferenceFrame(EConstraintFrame::Frame1, InitialFrame1);
	SetConstraintReferenceFrame(EConstraintFrame::Frame2, Frame2);
	OnGrabbedComponentChanged.Broadcast(GrabbedComponent);
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

	if (bWaitForGrabRelease && bGrabRequested)
	{
		GrabRetryCooldownRemaining = FMath::Max(
			GrabRetryCooldownRemaining - DeltaTime,
			0.0f);
		if (GrabRetryCooldownRemaining <= 0.0f)
		{
			bWaitForGrabRelease = false;
		}
	}

	if (bGrabSimulationEnabled
		&& bGrabRequested
		&& !bWaitForGrabRelease
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

	// 디버그 전용: 현재 Grab 범위와 힘 정보를 월드에 표시합니다.
	DrawGrabDebug();
}

void UNPStablePhysicsGrabComponent::HandleConstraintBroken(int32)
{
	if (!IsHoldingObject())
	{
		return;
	}

	bWaitForGrabRelease = true;
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
	GrabbedComponent = PrimitiveComponent;
	GrabbedGrabbableComponent = GrabbableComponent;
	GrabbedBoneName = NAME_None;
	// 디버그 전용: 새로운 Grab 대상의 디버그 누적 상태를 초기화합니다.
	ResetGrabDebug();

	SetWorldLocation(PhysicsMesh->GetSocketLocation(HandBoneName));

	// Constraint를 통해 손 Body와 물체가 서로 물리적인 힘을 주고받습니다.
	SetConstrainedComponents(
		PhysicsMesh,
		HandBoneName,
		GrabbedComponent,
		NAME_None);
	if (!ConstraintInstance.IsValidConstraintInstance())
	{
		GrabbedComponent = nullptr;
		GrabbedGrabbableComponent = nullptr;
		GrabbedBoneName = NAME_None;
		return false;
	}

	GrabbedGrabbableComponent->NotifyGrabStarted(GrabbedComponent);
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
	// 디버그 전용: 힘과 유저 의도를 평활화하고 표시용 일치도를 갱신합니다.
	UpdateGrabDebug(DeltaTime, RelicForce, UserIntent);

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

#pragma region Grab Debug Functions
void UNPStablePhysicsGrabComponent::ResetGrabDebug()
{
	LastDebugRelicForce = FVector::ZeroVector;
	SmoothedDebugUserIntent = FVector::ZeroVector;
	LastDebugIntentForceAlignment = 0.0f;
}
void UNPStablePhysicsGrabComponent::UpdateGrabDebug(
	float DeltaTime,
	const FVector& RelicForce,
	const FVector& UserIntent)
{
	if (RelicForce.Size() >= GrabDebugMinimumForce)
	{
		if (LastDebugRelicForce.IsNearlyZero())
		{
			LastDebugRelicForce = RelicForce;
		}
		else
		{
			const float ForceSmoothingAlpha = 1.0f - FMath::Exp(
				-GrabDebugForceSmoothingSpeed * DeltaTime);
			LastDebugRelicForce = FMath::Lerp(
				LastDebugRelicForce,
				RelicForce,
				ForceSmoothingAlpha);
		}
	}

	const float IntentSmoothingAlpha = 1.0f - FMath::Exp(
		-GrabDebugIntentSmoothingSpeed * DeltaTime);
	SmoothedDebugUserIntent = FMath::Lerp(
		SmoothedDebugUserIntent,
		UserIntent,
		IntentSmoothingAlpha);
	if (!SmoothedDebugUserIntent.IsNearlyZero()
		&& !LastDebugRelicForce.IsNearlyZero())
	{
		LastDebugIntentForceAlignment = FVector::DotProduct(
			SmoothedDebugUserIntent.GetSafeNormal(),
			LastDebugRelicForce.GetSafeNormal());
		return;
	}

	LastDebugIntentForceAlignment = 0.0f;
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

		if (GrabbedGrabbableComponent)
		{
			// 디버그 전용: 잡은 지점에 힘과 의도 방향을 표시합니다.
			DrawGrabForceDebug(HandLocation);
		}
	}
}
void UNPStablePhysicsGrabComponent::DrawGrabForceDebug(
	const FVector& ForceStart) const
{
	const FVector ForceDirection = LastDebugRelicForce.GetSafeNormal();
	const FVector IntentDirection =
		SmoothedDebugUserIntent.GetSafeNormal();
	if (!ForceDirection.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(
			GetWorld(),
			ForceStart,
			ForceStart + ForceDirection * 100.0f,
			20.0f,
			FColor::Cyan,
			false,
			0.0f,
			0,
			4.0f);
	}
	if (!IntentDirection.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(
			GetWorld(),
			ForceStart,
			ForceStart + SmoothedDebugUserIntent * 80.0f,
			16.0f,
			FColor::Yellow,
			false,
			0.0f,
			0,
			4.0f);
	}

	const float AlignedForce = FMath::Max(
		FVector::DotProduct(LastDebugRelicForce, IntentDirection),
		0.0f);
	FColor AlignmentColor = FColor::Red;
	if (LastDebugIntentForceAlignment >= 0.7f)
	{
		AlignmentColor = FColor::Green;
	}
	else if (LastDebugIntentForceAlignment > 0.0f)
	{
		AlignmentColor = FColor::Yellow;
	}

	const FString ForceText = FString::Printf(
		TEXT("Relic Force (Cyan): %.0f / %.0f\n")
		TEXT("User Intent (Yellow): %s\n")
		TEXT("Alignment: %.2f\n")
		TEXT("Aligned Force: %.0f\n%s"),
		LastDebugRelicForce.Size(),
		GrabLinearBreakThreshold,
		*SmoothedDebugUserIntent.ToCompactString(),
		LastDebugIntentForceAlignment,
		AlignedForce,
		*LastDebugRelicForce.ToCompactString());
	DrawDebugString(
		GetWorld(),
		ForceStart + FVector(0.0f, 0.0f, 30.0f),
		ForceText,
		nullptr,
		AlignmentColor,
		0.0f,
		false,
		1.0f);
}
#pragma endregion
