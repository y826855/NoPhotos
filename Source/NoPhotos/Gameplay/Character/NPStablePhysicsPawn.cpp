#include "Gameplay/Character/NPStablePhysicsPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Gameplay/Character/NPStablePhysicsGrabComponent.h"
#include "Gameplay/Character/NPStablePhysicsMovementComponent.h"

ANPStablePhysicsPawn::ANPStablePhysicsPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	PhysicsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PhysicsMesh"));
	SetRootComponent(PhysicsMesh);
	PhysicsMesh->SetCollisionProfileName(TEXT("Ragdoll"));
	PhysicsMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsMesh->SetEnableGravity(true);
	PhysicsMesh->PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::SimulationUpatesComponentTransform;
	PhysicsMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	CameraRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
	CameraRoot->SetupAttachment(PhysicsMesh);
	CameraRoot->SetUsingAbsoluteRotation(true);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(CameraRoot);
	CameraBoom->PrimaryComponentTick.TickGroup = TG_PostPhysics;
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 14.0f;
	CameraBoom->ProbeChannel = ECC_Camera;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 20.0f;
	CameraBoom->CameraLagMaxDistance = 30.0f;
	CameraBoom->bEnableCameraRotationLag = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	PhysicalAnimation = CreateDefaultSubobject<UPhysicalAnimationComponent>(TEXT("PhysicalAnimation"));
	PhysicsMovement = CreateDefaultSubobject<UNPStablePhysicsMovementComponent>(TEXT("PhysicsMovement"));

	RightHandGrab = CreateDefaultSubobject<UNPStablePhysicsGrabComponent>(TEXT("RightHandGrab"));
	RightHandGrab->SetupAttachment(PhysicsMesh);

	PhysicalBodyGroups = {
		FStablePhysicalBodyGroupSettings(TEXT("pelvis"), 2500.0f, 300.0f, 0.0f),
		FStablePhysicalBodyGroupSettings(TEXT("thigh_l"), 6000.0f, 700.0f, 0.0f),
		FStablePhysicalBodyGroupSettings(TEXT("thigh_r"), 6000.0f, 700.0f, 0.0f),
		FStablePhysicalBodyGroupSettings(TEXT("spine_02"), 700.0f, 100.0f, 0.0f),
		FStablePhysicalBodyGroupSettings(TEXT("neck_01"), 350.0f, 50.0f, 0.0f),
		FStablePhysicalBodyGroupSettings(TEXT("upperarm_l"), 250.0f, 35.0f, 0.0f),
		FStablePhysicalBodyGroupSettings(TEXT("upperarm_r"), 250.0f, 35.0f, 0.0f),
		FStablePhysicalBodyGroupSettings(TEXT("hand_l"), 80.0f, 15.0f, 0.0f),
		FStablePhysicalBodyGroupSettings(TEXT("hand_r"), 80.0f, 15.0f, 0.0f)
	};
}

void ANPStablePhysicsPawn::BeginPlay()
{
	Super::BeginPlay();

	ApplyCharacterProfile();
	PhysicsMovement->Initialize(PhysicsMesh, CharacterForwardYawOffset);
	RightHandGrab->Initialize(PhysicsMesh, RightHandBoneName);
	PhysicsMovement->OnJumpApplied.AddUObject(
		RightHandGrab,
		&UNPStablePhysicsGrabComponent::NotifyJumpIntent);
	InitializePhysicalAnimation();
	UpdateCameraTarget();
	CameraBoom->AddTickPrerequisiteActor(this);
}

void ANPStablePhysicsPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RefreshCharacterProfileIfChanged();
	UpdateCameraTarget();
	DrawFacingDebug();
	DrawPhysicalProfileDebug();
	UpdateRightHandIK(DeltaSeconds);
}

FVector ANPStablePhysicsPawn::GetVelocity() const
{
	return PhysicsMovement ? PhysicsMovement->GetVelocity() : FVector::ZeroVector;
}

float ANPStablePhysicsPawn::GetAnimationGroundSpeed() const
{
	FVector HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.0f;
	const float GroundSpeed = HorizontalVelocity.Size();
	return GroundSpeed >= AnimationSpeedDeadZone ? GroundSpeed : 0.0f;
}

float ANPStablePhysicsPawn::GetAnimationMovementDirection() const
{
	FVector HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.0f;
	if (HorizontalVelocity.SizeSquared() < FMath::Square(AnimationSpeedDeadZone))
	{
		return 0.0f;
	}

	const FVector MovementDirection = HorizontalVelocity.GetSafeNormal();
	const FVector VisualForward = GetVisualFacingRotation().Vector();
	const FVector VisualRight = FVector::CrossProduct(FVector::UpVector, VisualForward);
	return FMath::RadiansToDegrees(FMath::Atan2(
		FVector::DotProduct(MovementDirection, VisualRight),
		FVector::DotProduct(MovementDirection, VisualForward)));
}

FRotator ANPStablePhysicsPawn::GetVisualFacingRotation() const
{
	return GetVisualForwardDirection().Rotation();
}

FVector ANPStablePhysicsPawn::GetVisualForwardDirection() const
{
	return PhysicsMovement
		? PhysicsMovement->GetCurrentFacingDirection()
		: FVector::ForwardVector;
}

void ANPStablePhysicsPawn::ApplyCharacterProfile()
{
	if (!CharacterProfile)
	{
		return;
	}

	FullBodyRootName = CharacterProfile->PelvisBoneName;
	RightShoulderBoneName = CharacterProfile->RightShoulderBoneName;
	RightHandBoneName = CharacterProfile->RightHandBoneName;
	CharacterForwardYawOffset = CharacterProfile->CharacterForwardYawOffset;
	PhysicalBodyGroups = CharacterProfile->GetPhysicalBodyGroups();
	AppliedCharacterProfileRevision = CharacterProfile->GetSettingsRevision();
	AppliedCharacterProfile = CharacterProfile;

	PhysicsMovement->ConfigureBoneNames(
		CharacterProfile->PelvisBoneName,
		CharacterProfile->LeftFootBoneName,
		CharacterProfile->RightFootBoneName);
	PhysicsMovement->SetTargetPelvisHeight(CharacterProfile->PelvisHeight);
	PhysicsMovement->SetMaxMoveSpeed(CharacterProfile->MaxMoveSpeed);
	PhysicsMovement->SetJumpVelocityChange(CharacterProfile->JumpVelocityChange);
	RightHandGrab->SetLinearBreakThreshold(
		CharacterProfile->GrabLinearBreakThreshold);
}

void ANPStablePhysicsPawn::RefreshCharacterProfileIfChanged()
{
	if (!CharacterProfile
		|| (AppliedCharacterProfile.Get() == CharacterProfile
			&& AppliedCharacterProfileRevision == CharacterProfile->GetSettingsRevision()))
	{
		return;
	}

	// PIE 중 프로필을 조정해도 디버그 표시와 실제 Physical Animation을 함께 갱신합니다.
	ApplyCharacterProfile();
	PhysicsMovement->Initialize(PhysicsMesh, CharacterForwardYawOffset);
	RightHandGrab->Initialize(PhysicsMesh, RightHandBoneName);
	InitializePhysicalAnimation();
}

void ANPStablePhysicsPawn::InitializePhysicalAnimation()
{
	const UPhysicsAsset* PhysicsAsset = PhysicsMesh->GetPhysicsAsset();
	if (!PhysicsAsset
		|| PhysicsMesh->GetBoneIndex(FullBodyRootName) == INDEX_NONE
		|| PhysicsAsset->FindBodyIndex(FullBodyRootName) == INDEX_NONE)
	{
		return;
	}

	PhysicalAnimation->SetSkeletalMeshComponent(PhysicsMesh);
	ApplyPhysicalAnimationGroups();
	PhysicsMesh->SetAllBodiesBelowSimulatePhysics(FullBodyRootName, true, true);
	PhysicsMesh->WakeAllRigidBodies();
	ConfigurePelvisStability();
}

void ANPStablePhysicsPawn::ApplyPhysicalAnimationGroups()
{
	const UPhysicsAsset* PhysicsAsset = PhysicsMesh->GetPhysicsAsset();
	for (const FStablePhysicalBodyGroupSettings& Group : PhysicalBodyGroups)
	{
		if (Group.RootBodyName.IsNone()
			|| PhysicsMesh->GetBoneIndex(Group.RootBodyName) == INDEX_NONE
			|| !PhysicsAsset
			|| PhysicsAsset->FindBodyIndex(Group.RootBodyName) == INDEX_NONE)
		{
			continue;
		}

		FPhysicalAnimationData AnimationData;
		AnimationData.bIsLocalSimulation = true;
		AnimationData.OrientationStrength = Group.OrientationStrength;
		AnimationData.AngularVelocityStrength = Group.AngularVelocityStrength;
		AnimationData.MaxAngularForce = Group.MaxAngularForce;

		// Pelvis는 이동 제약으로 제어하고, 하위 Body는 뒤쪽 그룹 설정으로 덮어씁니다.
		const bool bIncludeRootBody = Group.RootBodyName != FullBodyRootName;
		PhysicalAnimation->ApplyPhysicalAnimationSettingsBelow(
			Group.RootBodyName,
			AnimationData,
			bIncludeRootBody);
	}
}

void ANPStablePhysicsPawn::ConfigurePelvisStability()
{
	if (FBodyInstance* PelvisBody = PhysicsMesh->GetBodyInstance(FullBodyRootName))
	{
		PelvisBody->bLockXRotation = bLockPelvisTilt;
		PelvisBody->bLockYRotation = bLockPelvisTilt;
		PelvisBody->bLockZRotation = !PhysicsMovement->IsFacingRotationEnabled();
		PelvisBody->SetDOFLock(EDOFMode::SixDOF);
	}
}

void ANPStablePhysicsPawn::UpdateCameraTarget()
{
	if (PhysicsMesh->GetBoneIndex(FullBodyRootName) == INDEX_NONE)
	{
		return;
	}

	const FVector TargetLocation = PhysicsMesh->GetSocketLocation(FullBodyRootName)
		+ FVector::UpVector * CameraTargetHeight;
	CameraRoot->SetWorldLocation(TargetLocation);
}

void ANPStablePhysicsPawn::DrawFacingDebug() const
{
	if (!bDrawFacingDebug
		|| !GetWorld()
		|| PhysicsMesh->GetBoneIndex(FullBodyRootName) == INDEX_NONE)
	{
		return;
	}

	const FVector ArrowStart = PhysicsMesh->GetSocketLocation(FullBodyRootName)
		+ FVector::UpVector * FacingDebugHeight;

	const float CameraYaw = Controller
		? Controller->GetControlRotation().Yaw
		: FollowCamera->GetComponentRotation().Yaw;
	const FVector CameraForward = FRotationMatrix(FRotator(0.0f, CameraYaw, 0.0f))
		.GetUnitAxis(EAxis::X);

	const FVector CharacterForward = GetVisualForwardDirection();

	DrawDebugDirectionalArrow(
		GetWorld(),
		ArrowStart + FVector::UpVector * 8.0f,
		ArrowStart + FVector::UpVector * 8.0f + CameraForward * FacingDebugArrowLength,
		20.0f,
		FColor::Green,
		false,
		0.0f,
		0,
		3.0f);

	DrawDebugDirectionalArrow(
		GetWorld(),
		ArrowStart - FVector::UpVector * 8.0f,
		ArrowStart - FVector::UpVector * 8.0f + CharacterForward * FacingDebugArrowLength,
		20.0f,
		FColor::Red,
		false,
		0.0f,
		0,
		3.0f);

	if (PhysicsMovement->HasFacingDirection())
	{
		DrawDebugDirectionalArrow(
			GetWorld(),
			ArrowStart,
			ArrowStart + PhysicsMovement->GetFacingDirection() * FacingDebugArrowLength,
			20.0f,
			FColor::Blue,
			false,
			0.0f,
			0,
			3.0f);
	}
}

void ANPStablePhysicsPawn::DrawPhysicalProfileDebug() const
{
	if (!CharacterProfile
		|| !CharacterProfile->bDrawPhysicalRegionDebug
		|| !GetWorld())
	{
		return;
	}

	const float LowerBodyRigidity = CharacterProfile->LowerBodyRigidity;
	const float TorsoRigidity = CharacterProfile->TorsoRigidity;
	const float HeadRigidity = CharacterProfile->HeadRigidity;
	const float ArmRigidity = CharacterProfile->ArmRigidity;
	const float HandRigidity = CharacterProfile->HandRigidity;

	DrawPhysicalProfileLink(
		CharacterProfile->PelvisBoneName,
		CharacterProfile->LeftThighBoneName,
		FColor::Blue,
		LowerBodyRigidity);
	DrawPhysicalProfileLink(
		CharacterProfile->PelvisBoneName,
		CharacterProfile->RightThighBoneName,
		FColor::Blue,
		LowerBodyRigidity);
	DrawPhysicalProfileLink(
		CharacterProfile->LeftThighBoneName,
		CharacterProfile->LeftFootBoneName,
		FColor::Blue,
		LowerBodyRigidity);
	DrawPhysicalProfileLink(
		CharacterProfile->RightThighBoneName,
		CharacterProfile->RightFootBoneName,
		FColor::Blue,
		LowerBodyRigidity);
	DrawPhysicalProfileLink(
		CharacterProfile->PelvisBoneName,
		CharacterProfile->SpineBoneName,
		FColor::Green,
		TorsoRigidity);
	DrawPhysicalProfileLink(
		CharacterProfile->SpineBoneName,
		CharacterProfile->NeckBoneName,
		FColor::Purple,
		HeadRigidity);
	DrawPhysicalProfileLink(
		CharacterProfile->SpineBoneName,
		CharacterProfile->LeftUpperArmBoneName,
		FColor::Orange,
		ArmRigidity);
	DrawPhysicalProfileLink(
		CharacterProfile->SpineBoneName,
		CharacterProfile->RightUpperArmBoneName,
		FColor::Orange,
		ArmRigidity);
	DrawPhysicalProfileLink(
		CharacterProfile->LeftUpperArmBoneName,
		CharacterProfile->LeftHandBoneName,
		FColor::Orange,
		ArmRigidity);
	DrawPhysicalProfileLink(
		CharacterProfile->RightUpperArmBoneName,
		CharacterProfile->RightHandBoneName,
		FColor::Orange,
		ArmRigidity);

	DrawPhysicalProfileBone(
		CharacterProfile->PelvisBoneName,
		FColor::Blue,
		LowerBodyRigidity,
		FString::Printf(TEXT("하체 %.0f"), LowerBodyRigidity));
	DrawPhysicalProfileBone(
		CharacterProfile->LeftThighBoneName,
		FColor::Blue,
		LowerBodyRigidity,
		FString());
	DrawPhysicalProfileBone(
		CharacterProfile->RightThighBoneName,
		FColor::Blue,
		LowerBodyRigidity,
		FString());
	DrawPhysicalProfileBone(
		CharacterProfile->LeftFootBoneName,
		FColor::Blue,
		LowerBodyRigidity,
		FString());
	DrawPhysicalProfileBone(
		CharacterProfile->RightFootBoneName,
		FColor::Blue,
		LowerBodyRigidity,
		FString());
	DrawPhysicalProfileBone(
		CharacterProfile->SpineBoneName,
		FColor::Green,
		TorsoRigidity,
		FString::Printf(TEXT("몸통 %.0f"), TorsoRigidity));
	DrawPhysicalProfileBone(
		CharacterProfile->NeckBoneName,
		FColor::Purple,
		HeadRigidity,
		FString::Printf(TEXT("목/머리 %.0f"), HeadRigidity));
	DrawPhysicalProfileBone(
		CharacterProfile->LeftUpperArmBoneName,
		FColor::Orange,
		ArmRigidity,
		FString());
	DrawPhysicalProfileBone(
		CharacterProfile->RightUpperArmBoneName,
		FColor::Orange,
		ArmRigidity,
		FString::Printf(TEXT("팔 %.0f"), ArmRigidity));
	DrawPhysicalProfileBone(
		CharacterProfile->LeftHandBoneName,
		FColor::Red,
		HandRigidity,
		FString());
	DrawPhysicalProfileBone(
		CharacterProfile->RightHandBoneName,
		FColor::Red,
		HandRigidity,
		FString::Printf(TEXT("손 %.0f"), HandRigidity));
}

void ANPStablePhysicsPawn::DrawPhysicalProfileBone(
	FName BoneName,
	const FColor& Color,
	float Rigidity,
	const FString& Label) const
{
	if (PhysicsMesh->GetBoneIndex(BoneName) == INDEX_NONE)
	{
		return;
	}

	const float ClampedRigidity = FMath::Clamp(Rigidity, 0.0f, 100.0f);
	const FVector BoneLocation = PhysicsMesh->GetSocketLocation(BoneName);
	DrawDebugSphere(
		GetWorld(),
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
			GetWorld(),
			BoneLocation + FVector::UpVector * 14.0f,
			Label,
			nullptr,
			Color,
			0.0f,
			false,
			0.85f);
	}
}

void ANPStablePhysicsPawn::DrawPhysicalProfileLink(
	FName StartBoneName,
	FName EndBoneName,
	const FColor& Color,
	float Rigidity) const
{
	if (PhysicsMesh->GetBoneIndex(StartBoneName) == INDEX_NONE
		|| PhysicsMesh->GetBoneIndex(EndBoneName) == INDEX_NONE)
	{
		return;
	}

	const float ClampedRigidity = FMath::Clamp(Rigidity, 0.0f, 100.0f);
	DrawDebugLine(
		GetWorld(),
		PhysicsMesh->GetSocketLocation(StartBoneName),
		PhysicsMesh->GetSocketLocation(EndBoneName),
		Color,
		false,
		0.0f,
		0,
		1.0f + ClampedRigidity * 0.04f);
}

void ANPStablePhysicsPawn::UpdateRightHandIK(float DeltaSeconds)
{
	const float TargetAlpha = bRightHandActive ? 1.0f : 0.0f;
	RightHandIKAlpha = FMath::FInterpTo(
		RightHandIKAlpha,
		TargetAlpha,
		DeltaSeconds,
		RightHandIKBlendSpeed);

	if (PhysicsMesh->GetBoneIndex(RightShoulderBoneName) == INDEX_NONE)
	{
		return;
	}

	const FVector ShoulderLocation = PhysicsMesh->GetSocketLocation(RightShoulderBoneName);
	const FRotator FacingRotation = GetVisualFacingRotation();
	const FVector CharacterForward = FacingRotation.Vector();
	const FVector CharacterRight = FRotationMatrix(FacingRotation).GetUnitAxis(EAxis::Y);
	const FVector HandTargetLocation = ShoulderLocation + CharacterForward * RightHandReachDistance;
	const FVector ElbowTargetLocation = ShoulderLocation
		+ CharacterForward * (RightHandReachDistance * 0.5f)
		+ CharacterRight * RightElbowOutwardDistance;

	const FTransform& MeshTransform = PhysicsMesh->GetComponentTransform();
	RightHandIKLocation = MeshTransform.InverseTransformPosition(HandTargetLocation);
	RightElbowIKLocation = MeshTransform.InverseTransformPosition(ElbowTargetLocation);
}

void ANPStablePhysicsPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANPStablePhysicsPawn::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ANPStablePhysicsPawn::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ANPStablePhysicsPawn::Move);
	}
	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANPStablePhysicsPawn::Look);
	}
	if (MouseLookAction)
	{
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ANPStablePhysicsPawn::Look);
	}
	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ANPStablePhysicsPawn::Jump);
	}
	if (RightHandAction)
	{
		EnhancedInputComponent->BindAction(RightHandAction, ETriggerEvent::Started, this, &ANPStablePhysicsPawn::StartRightHand);
		EnhancedInputComponent->BindAction(RightHandAction, ETriggerEvent::Completed, this, &ANPStablePhysicsPawn::StopRightHand);
		EnhancedInputComponent->BindAction(RightHandAction, ETriggerEvent::Canceled, this, &ANPStablePhysicsPawn::StopRightHand);
	}
}

void ANPStablePhysicsPawn::Move(const FInputActionValue& Value)
{
	const FVector2D MovementInput = Value.Get<FVector2D>();
	if (!Controller)
	{
		ApplyMoveInput(FVector::ZeroVector);
		return;
	}

	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	ApplyMoveInput(
		ForwardDirection * MovementInput.Y
		+ RightDirection * MovementInput.X);
}

void ANPStablePhysicsPawn::Look(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void ANPStablePhysicsPawn::Jump()
{
	ApplyJumpRequest();
}

void ANPStablePhysicsPawn::StartRightHand()
{
	ApplyRightHandState(true);
}

void ANPStablePhysicsPawn::StopRightHand()
{
	ApplyRightHandState(false);
}

void ANPStablePhysicsPawn::ApplyMoveInput(const FVector& WorldMoveInput)
{
	PhysicsMovement->SetMoveInput(WorldMoveInput);
}

void ANPStablePhysicsPawn::ApplyJumpRequest()
{
	PhysicsMovement->RequestJump();
}

void ANPStablePhysicsPawn::ApplyRightHandState(bool bActive)
{
	SetRightHandVisualState(bActive);
	RightHandGrab->SetGrabRequested(bActive);
}

void ANPStablePhysicsPawn::SetRightHandVisualState(bool bActive)
{
	bRightHandActive = bActive;
}
