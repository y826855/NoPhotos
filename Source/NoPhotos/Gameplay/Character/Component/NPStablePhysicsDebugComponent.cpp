#include "Gameplay/Character/Component/NPStablePhysicsDebugComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Gameplay/Character/NPRStablePhysicsPawn.h"
#include "Gameplay/Character/NPStablePhysicsCharacterProfile.h"
#include "Gameplay/Character/Component/NPStablePhysicsGrabComponent.h"
#include "Gameplay/Character/Component/NPStablePhysicsMovementComponent.h"

UNPStablePhysicsDebugComponent::UNPStablePhysicsDebugComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UNPStablePhysicsDebugComponent::ResetGrabDebug()
{
	LastGrabRelicForce = FVector::ZeroVector;
	SmoothedGrabUserIntent = FVector::ZeroVector;
	LastGrabIntentForceAlignment = 0.0f;
}

void UNPStablePhysicsDebugComponent::UpdateGrabDebug(
	const UNPStablePhysicsGrabComponent& GrabComponent,
	float DeltaTime,
	const FVector& RelicForce,
	const FVector& UserIntent)
{
	if (RelicForce.Size() >= GrabComponent.GrabDebugMinimumForce)
	{
		if (LastGrabRelicForce.IsNearlyZero())
		{
			LastGrabRelicForce = RelicForce;
		}
		else
		{
			const float ForceSmoothingAlpha = 1.0f - FMath::Exp(
				-GrabComponent.GrabDebugForceSmoothingSpeed * DeltaTime);
			LastGrabRelicForce = FMath::Lerp(
				LastGrabRelicForce,
				RelicForce,
				ForceSmoothingAlpha);
		}
	}

	const float IntentSmoothingAlpha = 1.0f - FMath::Exp(
		-GrabComponent.GrabDebugIntentSmoothingSpeed * DeltaTime);
	SmoothedGrabUserIntent = FMath::Lerp(
		SmoothedGrabUserIntent,
		UserIntent,
		IntentSmoothingAlpha);
	if (!SmoothedGrabUserIntent.IsNearlyZero()
		&& !LastGrabRelicForce.IsNearlyZero())
	{
		LastGrabIntentForceAlignment = FVector::DotProduct(
			SmoothedGrabUserIntent.GetSafeNormal(),
			LastGrabRelicForce.GetSafeNormal());
		return;
	}

	LastGrabIntentForceAlignment = 0.0f;
}

void UNPStablePhysicsDebugComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const ANPStablePhysicsPawn* Pawn = Cast<ANPStablePhysicsPawn>(GetOwner());
	if (!Pawn)
	{
		return;
	}

	DrawFacingDebug(*Pawn);
	DrawPhysicalProfileDebug(*Pawn);
	if (Pawn->RightHandGrab)
	{
		DrawGrabDebug(*Pawn->RightHandGrab);
	}
	DrawGrabNetworkDebug(*Pawn);
}

void UNPStablePhysicsDebugComponent::DrawFacingDebug(
	const ANPStablePhysicsPawn& Pawn) const
{
	if (!Pawn.bDrawFacingDebug
		|| !Pawn.GetWorld()
		|| Pawn.PhysicsMesh->GetBoneIndex(Pawn.FullBodyRootName) == INDEX_NONE)
	{
		return;
	}

	const FVector ArrowStart = Pawn.PhysicsMesh->GetSocketLocation(
		Pawn.FullBodyRootName) + FVector::UpVector * Pawn.FacingDebugHeight;
	const float CameraYaw = Pawn.GetTargetViewRotation().Yaw;
	const FVector CameraForward = FRotationMatrix(FRotator(0.0f, CameraYaw, 0.0f))
		.GetUnitAxis(EAxis::X);
	const FVector CharacterForward = Pawn.GetVisualForwardDirection();

	DrawDebugDirectionalArrow(
		Pawn.GetWorld(),
		ArrowStart + FVector::UpVector * 8.0f,
		ArrowStart + FVector::UpVector * 8.0f
			+ CameraForward * Pawn.FacingDebugArrowLength,
		20.0f,
		FColor::Green,
		false,
		0.0f,
		0,
		3.0f);
	DrawDebugDirectionalArrow(
		Pawn.GetWorld(),
		ArrowStart - FVector::UpVector * 8.0f,
		ArrowStart - FVector::UpVector * 8.0f
			+ CharacterForward * Pawn.FacingDebugArrowLength,
		20.0f,
		FColor::Red,
		false,
		0.0f,
		0,
		3.0f);

	if (Pawn.PhysicsMovement->HasFacingDirection())
	{
		DrawDebugDirectionalArrow(
			Pawn.GetWorld(),
			ArrowStart,
			ArrowStart + Pawn.PhysicsMovement->GetFacingDirection()
				* Pawn.FacingDebugArrowLength,
			20.0f,
			FColor::Blue,
			false,
			0.0f,
			0,
			3.0f);
	}
}

void UNPStablePhysicsDebugComponent::DrawPhysicalProfileDebug(
	const ANPStablePhysicsPawn& Pawn) const
{
	const UNPStablePhysicsCharacterProfile* Profile = Pawn.CharacterProfile;
	if (!Profile || !Profile->bDrawPhysicalRegionDebug || !Pawn.GetWorld())
	{
		return;
	}

	DrawPhysicalProfileLink(
		Pawn, Profile->PelvisBoneName, Profile->LeftThighBoneName,
		FColor::Blue, Profile->LowerBodyRigidity);
	DrawPhysicalProfileLink(
		Pawn, Profile->PelvisBoneName, Profile->RightThighBoneName,
		FColor::Blue, Profile->LowerBodyRigidity);
	DrawPhysicalProfileLink(
		Pawn, Profile->LeftThighBoneName, Profile->LeftFootBoneName,
		FColor::Blue, Profile->LowerBodyRigidity);
	DrawPhysicalProfileLink(
		Pawn, Profile->RightThighBoneName, Profile->RightFootBoneName,
		FColor::Blue, Profile->LowerBodyRigidity);
	DrawPhysicalProfileLink(Pawn, Profile->PelvisBoneName, Profile->SpineBoneName, FColor::Green, Profile->TorsoRigidity);
	DrawPhysicalProfileLink(Pawn, Profile->SpineBoneName, Profile->NeckBoneName, FColor::Purple, Profile->HeadRigidity);
	DrawPhysicalProfileLink(
		Pawn, Profile->SpineBoneName, Profile->LeftUpperArmBoneName,
		FColor::Orange, Profile->ArmRigidity);
	DrawPhysicalProfileLink(
		Pawn, Profile->SpineBoneName, Profile->RightUpperArmBoneName,
		FColor::Orange, Profile->ArmRigidity);
	DrawPhysicalProfileLink(
		Pawn, Profile->LeftUpperArmBoneName, Profile->LeftHandBoneName,
		FColor::Orange, Profile->ArmRigidity);
	DrawPhysicalProfileLink(
		Pawn, Profile->RightUpperArmBoneName, Profile->RightHandBoneName,
		FColor::Orange, Profile->ArmRigidity);

	DrawPhysicalProfileBone(
		Pawn, Profile->PelvisBoneName, FColor::Blue,
		Profile->LowerBodyRigidity,
		FString::Printf(TEXT("하체 %.0f"), Profile->LowerBodyRigidity));
	DrawPhysicalProfileBone(Pawn, Profile->LeftThighBoneName, FColor::Blue, Profile->LowerBodyRigidity, FString());
	DrawPhysicalProfileBone(Pawn, Profile->RightThighBoneName, FColor::Blue, Profile->LowerBodyRigidity, FString());
	DrawPhysicalProfileBone(Pawn, Profile->LeftFootBoneName, FColor::Blue, Profile->LowerBodyRigidity, FString());
	DrawPhysicalProfileBone(Pawn, Profile->RightFootBoneName, FColor::Blue, Profile->LowerBodyRigidity, FString());
	DrawPhysicalProfileBone(
		Pawn, Profile->SpineBoneName, FColor::Green,
		Profile->TorsoRigidity,
		FString::Printf(TEXT("몸통 %.0f"), Profile->TorsoRigidity));
	DrawPhysicalProfileBone(
		Pawn, Profile->NeckBoneName, FColor::Purple,
		Profile->HeadRigidity,
		FString::Printf(TEXT("목/머리 %.0f"), Profile->HeadRigidity));
	DrawPhysicalProfileBone(Pawn, Profile->LeftUpperArmBoneName, FColor::Orange, Profile->ArmRigidity, FString());
	DrawPhysicalProfileBone(
		Pawn, Profile->RightUpperArmBoneName, FColor::Orange,
		Profile->ArmRigidity,
		FString::Printf(TEXT("팔 %.0f"), Profile->ArmRigidity));
	DrawPhysicalProfileBone(Pawn, Profile->LeftHandBoneName, FColor::Red, Profile->HandRigidity, FString());
	DrawPhysicalProfileBone(
		Pawn, Profile->RightHandBoneName, FColor::Red,
		Profile->HandRigidity,
		FString::Printf(TEXT("손 %.0f"), Profile->HandRigidity));
}

void UNPStablePhysicsDebugComponent::DrawPhysicalProfileBone(
	const ANPStablePhysicsPawn& Pawn,
	FName BoneName,
	const FColor& Color,
	float Rigidity,
	const FString& Label) const
{
	if (Pawn.PhysicsMesh->GetBoneIndex(BoneName) == INDEX_NONE)
	{
		return;
	}

	const float ClampedRigidity = FMath::Clamp(Rigidity, 0.0f, 100.0f);
	const FVector BoneLocation = Pawn.PhysicsMesh->GetSocketLocation(BoneName);
	DrawDebugSphere(
		Pawn.GetWorld(),
		BoneLocation,
		4.0f + ClampedRigidity * 0.08f,
		12,
		Color,
		false,
		0.0f,
		0,
		1.0f + ClampedRigidity * 0.02f);

	if (!Label.IsEmpty())
	{
		DrawDebugString(
			Pawn.GetWorld(),
			BoneLocation + FVector::UpVector * 14.0f,
			Label,
			nullptr,
			Color,
			0.0f,
			false,
			0.85f);
	}
}

void UNPStablePhysicsDebugComponent::DrawPhysicalProfileLink(
	const ANPStablePhysicsPawn& Pawn,
	FName StartBoneName,
	FName EndBoneName,
	const FColor& Color,
	float Rigidity) const
{
	if (Pawn.PhysicsMesh->GetBoneIndex(StartBoneName) == INDEX_NONE
		|| Pawn.PhysicsMesh->GetBoneIndex(EndBoneName) == INDEX_NONE)
	{
		return;
	}

	const float ClampedRigidity = FMath::Clamp(Rigidity, 0.0f, 100.0f);
	DrawDebugLine(
		Pawn.GetWorld(),
		Pawn.PhysicsMesh->GetSocketLocation(StartBoneName),
		Pawn.PhysicsMesh->GetSocketLocation(EndBoneName),
		Color,
		false,
		0.0f,
		0,
		1.0f + ClampedRigidity * 0.04f);
}

void UNPStablePhysicsDebugComponent::DrawGrabDebug(
	const UNPStablePhysicsGrabComponent& GrabComponent) const
{
	if (!GrabComponent.bDrawGrabDebug
		|| !GrabComponent.PhysicsMesh
		|| !GrabComponent.GetWorld()
		|| GrabComponent.PhysicsMesh->GetBoneIndex(GrabComponent.HandBoneName)
			== INDEX_NONE)
	{
		return;
	}

	const FVector HandLocation = GrabComponent.PhysicsMesh->GetSocketLocation(
		GrabComponent.HandBoneName);
	DrawDebugSphere(
		GrabComponent.GetWorld(),
		HandLocation,
		GrabComponent.GrabRadius,
		16,
		GrabComponent.IsHoldingObject() ? FColor::Green : FColor::Yellow,
		false,
		0.0f,
		0,
		2.0f);

	if (!GrabComponent.IsHoldingObject())
	{
		return;
	}

	DrawDebugLine(
		GrabComponent.GetWorld(),
		HandLocation,
		GrabComponent.GrabbedComponent->GetComponentLocation(),
		FColor::Green,
		false,
		0.0f,
		0,
		3.0f);
	if (GrabComponent.GrabbedGrabbableComponent)
	{
		DrawGrabForceDebug(GrabComponent, HandLocation);
	}
}

void UNPStablePhysicsDebugComponent::DrawGrabForceDebug(
	const UNPStablePhysicsGrabComponent& GrabComponent,
	const FVector& ForceStart) const
{
	const FVector ForceDirection = LastGrabRelicForce.GetSafeNormal();
	const FVector IntentDirection = SmoothedGrabUserIntent.GetSafeNormal();
	if (!ForceDirection.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(
			GrabComponent.GetWorld(),
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
			GrabComponent.GetWorld(),
			ForceStart,
			ForceStart + SmoothedGrabUserIntent * 80.0f,
			16.0f,
			FColor::Yellow,
			false,
			0.0f,
			0,
			4.0f);
	}

	const float AlignedForce = FMath::Max(
		FVector::DotProduct(LastGrabRelicForce, IntentDirection),
		0.0f);
	FColor AlignmentColor = FColor::Red;
	if (LastGrabIntentForceAlignment >= 0.7f)
	{
		AlignmentColor = FColor::Green;
	}
	else if (LastGrabIntentForceAlignment > 0.0f)
	{
		AlignmentColor = FColor::Yellow;
	}

	const FString ForceText = FString::Printf(
		TEXT("Relic Force (Cyan): %.0f / %.0f\n")
		TEXT("User Intent (Yellow): %s\n")
		TEXT("Alignment: %.2f\n")
		TEXT("Aligned Force: %.0f\n%s"),
		LastGrabRelicForce.Size(),
		GrabComponent.GrabLinearBreakThreshold,
		*SmoothedGrabUserIntent.ToCompactString(),
		LastGrabIntentForceAlignment,
		AlignedForce,
		*LastGrabRelicForce.ToCompactString());
	DrawDebugString(
		GrabComponent.GetWorld(),
		ForceStart + FVector(0.0f, 0.0f, 30.0f),
		ForceText,
		nullptr,
		AlignmentColor,
		0.0f,
		false,
		1.0f);
}

void UNPStablePhysicsDebugComponent::DrawGrabNetworkDebug(
	const ANPStablePhysicsPawn& Pawn) const
{
	const ANPRStablePhysicsPawn* NetworkPawn = Cast<ANPRStablePhysicsPawn>(&Pawn);
	if (!NetworkPawn
		|| !NetworkPawn->bDrawGrabNetworkDebug
		|| !NetworkPawn->IsReplicatedGrabActive()
		|| !NetworkPawn->GetWorld()
		|| NetworkPawn->PhysicsMesh->GetBoneIndex(NetworkPawn->RightHandBoneName)
			== INDEX_NONE)
	{
		return;
	}

	const FVector ServerHandLocation = FVector(
		NetworkPawn->ReplicatedServerHandWorldLocation);
	const FVector ClientHandLocation = NetworkPawn->PhysicsMesh->GetSocketLocation(
		NetworkPawn->RightHandBoneName);
	const float ErrorDistance = FVector::Distance(
		ServerHandLocation,
		ClientHandLocation);

	DrawDebugSphere(
		NetworkPawn->GetWorld(),
		ServerHandLocation,
		8.0f,
		12,
		FColor::Red,
		false,
		0.0f,
		0,
		2.0f);
	DrawDebugSphere(
		NetworkPawn->GetWorld(),
		ClientHandLocation,
		8.0f,
		12,
		FColor::Blue,
		false,
		0.0f,
		0,
		2.0f);
	DrawDebugLine(
		NetworkPawn->GetWorld(),
		ServerHandLocation,
		ClientHandLocation,
		FColor::Yellow,
		false,
		0.0f,
		0,
		2.0f);
	DrawDebugString(
		NetworkPawn->GetWorld(),
		(ServerHandLocation + ClientHandLocation) * 0.5f,
		FString::Printf(TEXT("Grab IK Error: %.1f cm"), ErrorDistance),
		nullptr,
		FColor::Yellow,
		0.0f,
		false,
		1.0f);
}
