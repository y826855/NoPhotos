#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Gameplay/Character/NPStablePhysicsCharacterProfile.h"
#include "NPStablePhysicsPawn.generated.h"

class UCameraComponent;
class UAnimMontage;
class UInputAction;
class UPhysicalAnimationComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
class UNPStablePhysicsGrabComponent;
class UNPStablePhysicsMovementComponent;
struct FInputActionValue;

/** 물리 캐릭터 프로토타입을 위한 안정적인 힘 기반 Pawn입니다. */
UCLASS()
class NOPHOTOS_API ANPStablePhysicsPawn : public APawn
{
	GENERATED_BODY()

public:
	ANPStablePhysicsPawn();

	virtual FVector GetVelocity() const override;
	/** 외부 게임 규칙이 현재 이동 의도를 즉시 제거할 때 사용합니다. */
	void StopMovementInput();
	/** 점프대처럼 외부 게임 규칙이 물리 캐릭터 전체에 즉시 속도 변화를 적용할 때 사용합니다. */
	void AddExternalVelocityChange(const FVector& VelocityChange);

	/** 사진 촬영으로 인한 이동 잠금만 변경합니다. Controller의 전역 입력 잠금과는 독립적입니다. */
	void SetPhotoMovementLocked(bool bLocked);

	UFUNCTION(BlueprintPure, Category="Photo")
	bool IsPhotoMovementLocked() const { return bPhotoMovementLocked; }

	/** 설정된 사진 촬영 Montage를 한 번 재생합니다. */
	bool PlayPhotoShotMontage();

	UPROPERTY(BlueprintReadOnly, Transient, Category="Right Hand IK")
	FVector RightHandIKLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Right Hand IK")
	FVector RightElbowIKLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Transient, Category="Right Hand IK")
	float RightHandIKAlpha = 0.0f;

	UFUNCTION(BlueprintPure, Category="Components")
	UNPStablePhysicsMovementComponent* GetStablePhysicsMovementComponent() const { return PhysicsMovement; }

	UFUNCTION(BlueprintPure, Category="Components")
	UNPStablePhysicsGrabComponent* GetRightHandGrabComponent() const { return RightHandGrab; }

	UFUNCTION(BlueprintPure, Category="Animation")
	float GetAnimationGroundSpeed() const;

	/** 보정된 캐릭터 정면을 기준으로 계산한 블렌드 스페이스용 이동 각도입니다. */
	UFUNCTION(BlueprintPure, Category="Animation")
	float GetAnimationMovementDirection() const;

	UFUNCTION(BlueprintPure, Category="Animation")
	FVector GetVisualForwardDirection() const;

	UFUNCTION(BlueprintPure, Category="Animation")
	FRotator GetVisualFacingRotation() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** 계산된 월드 이동 입력을 실제 Movement Component에 전달합니다. */
	virtual void ApplyMoveInput(const FVector& WorldMoveInput);

	/** 점프 입력을 실제 물리 점프 요청으로 전달합니다. */
	virtual void ApplyJumpRequest();

	/** 오른손의 시각 상태와 실제 그랩 요청을 함께 변경합니다. */
	virtual void ApplyRightHandState(bool bActive);

	/** 그랩 판정과 관계없이 오른손 IK에 사용할 시각 상태만 변경합니다. */
	void SetRightHandVisualState(bool bActive);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USkeletalMeshComponent* PhysicsMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USceneComponent* CameraRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UCameraComponent* FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UPhysicalAnimationComponent* PhysicalAnimation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UNPStablePhysicsGrabComponent* RightHandGrab;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UNPStablePhysicsMovementComponent* PhysicsMovement;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* RightHandAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation|Photo")
	TObjectPtr<UAnimMontage> PhotoShotMontage;

	/** 지정하면 본 이름과 피지컬 애니메이션 설정을 프로필 값으로 덮어씁니다. */
	UPROPERTY(EditAnywhere, Category="Character Profile")
	TObjectPtr<UNPStablePhysicsCharacterProfile> CharacterProfile;

	UPROPERTY(EditAnywhere, Category="Physical Animation")
	FName FullBodyRootName = TEXT("pelvis");

	UPROPERTY(EditAnywhere, Category="Physical Animation")
	bool bLockPelvisTilt = true;

	/** 프로필의 의미 기반 수치를 런타임 Physical Animation 설정으로 변환한 결과입니다. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category="Physical Animation", meta=(DisplayName="프로필 미사용 시 Body 그룹"))
	TArray<FStablePhysicalBodyGroupSettings> PhysicalBodyGroups;

	UPROPERTY(EditAnywhere, Category="Camera", meta=(ClampMin="0.0"))
	float CameraTargetHeight = 60.0f;

#pragma region Pawn Debug Settings
	UPROPERTY(EditAnywhere, Category="Facing Debug")
	bool bDrawFacingDebug = true;

	UPROPERTY(EditAnywhere, Category="Facing Debug", meta=(ClampMin="0.0"))
	float FacingDebugHeight = 100.0f;

	UPROPERTY(EditAnywhere, Category="Facing Debug", meta=(ClampMin="1.0"))
	float FacingDebugArrowLength = 150.0f;
#pragma endregion

	/** 메시의 로컬 +X축과 실제 캐릭터 정면 사이의 각도를 보정합니다. */
	UPROPERTY(EditAnywhere, Category="Facing", meta=(ClampMin="-180.0", ClampMax="180.0"))
	float CharacterForwardYawOffset = 90.0f;

	UPROPERTY(EditAnywhere, Category="Animation", meta=(ClampMin="0.0"))
	float AnimationSpeedDeadZone = 15.0f;

	UPROPERTY(EditAnywhere, Category="Right Hand IK")
	FName RightShoulderBoneName = TEXT("clavicle_r");

	UPROPERTY(EditAnywhere, Category="Right Hand IK")
	FName RightHandBoneName = TEXT("hand_r");

	UPROPERTY(EditAnywhere, Category="Right Hand IK", meta=(ClampMin="0.0"))
	float RightHandReachDistance = 120.0f;

	UPROPERTY(EditAnywhere, Category="Right Hand IK", meta=(ClampMin="0.0"))
	float RightElbowOutwardDistance = 40.0f;

	UPROPERTY(EditAnywhere, Category="Right Hand IK", meta=(ClampMin="0.0"))
	float RightHandIKBlendSpeed = 10.0f;

private:
	void ApplyCharacterProfile();
	void RefreshCharacterProfileIfChanged();
	void InitializePhysicalAnimation();
	void ApplyPhysicalAnimationGroups();
	void ConfigurePelvisStability();
	void UpdateCameraTarget();

	void UpdateRightHandIK(float DeltaSeconds);
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump();
	void StartRightHand();
	void StopRightHand();

#pragma region Pawn Debug Functions
	void DrawFacingDebug() const;
	void DrawPhysicalProfileDebug() const;
	void DrawPhysicalProfileBone(
		FName BoneName,
		const FColor& Color,
		float Rigidity,
		const FString& Label) const;
	void DrawPhysicalProfileLink(
		FName StartBoneName,
		FName EndBoneName,
		const FColor& Color,
		float Rigidity) const;
#pragma endregion


	bool bRightHandActive = false;
	bool bPhotoMovementLocked = false;
	uint32 AppliedCharacterProfileRevision = 0;
	TWeakObjectPtr<UNPStablePhysicsCharacterProfile> AppliedCharacterProfile;
};
