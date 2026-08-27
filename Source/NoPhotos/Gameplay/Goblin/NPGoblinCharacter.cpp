#include "NPGoblinCharacter.h"

#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/Photo/NPPhotoLog.h"
#include "Gameplay/Relic/NPBaseRelic.h"
#include "Net/UnrealNetwork.h"
#include "NPGoblinAIController.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogNPGoblinCharacter, Log, All);

ANPGoblinCharacter::ANPGoblinCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	AIControllerClass = ANPGoblinAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		Movement->MaxWalkSpeed = RoamMoveSpeed;
	}
}

void ANPGoblinCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		CurrentPhotoHP = GetMaxPhotoHP();
		ForceNetUpdate();
	}
	OnRep_CurrentPhotoHP();
	NotifyLifecycleStateChanged();
}

void ANPGoblinCharacter::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, LifecycleState);
	DOREPLIFETIME(ThisClass, CurrentPhotoHP);
}

void ANPGoblinCharacter::PrepareForSpawnPresentation()
{
	if (!HasAuthority())
	{
		return;
	}

	LifecycleState = ENPGoblinLifecycleState::Spawning;
	ForceNetUpdate();
}

void ANPGoblinCharacter::FinishSpawnPresentation()
{
	if (HasAuthority() && LifecycleState == ENPGoblinLifecycleState::Spawning)
	{
		SetLifecycleState(ENPGoblinLifecycleState::Active);
	}
}

void ANPGoblinCharacter::BeginDespawnPresentation()
{
	if (HasAuthority() && LifecycleState != ENPGoblinLifecycleState::Despawning)
	{
		SetLifecycleState(ENPGoblinLifecycleState::Despawning);
	}
}

void ANPGoblinCharacter::FinishDespawnPresentation()
{
	if (HasAuthority() && LifecycleState == ENPGoblinLifecycleState::Despawning)
	{
		Destroy();
	}
}

void ANPGoblinCharacter::SetLifecycleState(const ENPGoblinLifecycleState NewState)
{
	if (!HasAuthority() || LifecycleState == NewState)
	{
		return;
	}

	LifecycleState = NewState;
	NotifyLifecycleStateChanged();
	ForceNetUpdate();
}

void ANPGoblinCharacter::NotifyLifecycleStateChanged()
{
	if (LastNotifiedLifecycleState == LifecycleState)
	{
		return;
	}

	LastNotifiedLifecycleState = LifecycleState;
	GetWorldTimerManager().ClearTimer(PresentationTimeoutTimer);

	ANPGoblinAIController* GoblinController = Cast<ANPGoblinAIController>(GetController());
	const bool bShouldEnableGameplay = LifecycleState == ENPGoblinLifecycleState::Active;
	if (HasAuthority() && GoblinController)
	{
		GoblinController->SetGameplayEnabled(bShouldEnableGameplay);
	}

	switch (LifecycleState)
	{
	case ENPGoblinLifecycleState::Spawning:
		if (HasAuthority())
		{
			if (SpawnPresentationTimeout <= 0.0f)
			{
				FinishSpawnPresentation();
				return;
			}
			GetWorldTimerManager().SetTimer(
				PresentationTimeoutTimer,
				this,
				&ThisClass::FinishSpawnPresentation,
				SpawnPresentationTimeout,
				false);
		}
		BP_OnSpawnPresentationStarted();
		break;

	case ENPGoblinLifecycleState::Active:
		BP_OnGameplayActivated();
		break;

	case ENPGoblinLifecycleState::Despawning:
		if (HasAuthority())
		{
			if (DespawnPresentationTimeout <= 0.0f)
			{
				FinishDespawnPresentation();
				return;
			}
			GetWorldTimerManager().SetTimer(
				PresentationTimeoutTimer,
				this,
				&ThisClass::FinishDespawnPresentation,
				DespawnPresentationTimeout,
				false);
		}
		BP_OnDespawnPresentationStarted();
		break;

	default:
		break;
	}
}

void ANPGoblinCharacter::OnRep_LifecycleState()
{
	NotifyLifecycleStateChanged();
}

void ANPGoblinCharacter::OnRep_CurrentPhotoHP()
{
	BP_OnPhotoHPChanged(CurrentPhotoHP, GetMaxPhotoHP());
}

bool ANPGoblinCharacter::CanBePhotographed_Implementation(APlayerState* Photographer) const
{
	return HasAuthority()
		&& IsGameplayActive()
		&& IsValid(Photographer)
		&& CurrentPhotoHP > 0;
}

void ANPGoblinCharacter::OnPhotographed_Implementation(
	APlayerState* Photographer,
	const float Visibility,
	const int32 CaptureSequence)
{
	if (!HasAuthority()
		|| !IsGameplayActive()
		|| !IsValid(Photographer)
		|| CurrentPhotoHP <= 0)
	{
		return;
	}

	const int32 PreviousPhotoHP = CurrentPhotoHP;
	const int32 AppliedDamage = FMath::Min(
		CurrentPhotoHP,
		FMath::Max(1, PhotoDamagePerCapture));
	CurrentPhotoHP -= AppliedDamage;
	OnRep_CurrentPhotoHP();
	ForceNetUpdate();
	UE_LOG(
		LogNPPhoto,
		Display,
		TEXT("[GoblinPhoto] HIT Goblin=%s Photographer=%s Visibility=%.2f Sequence=%d Damage=%d HP=%d->%d/%d"),
		*GetNameSafe(this),
		*GetNameSafe(Photographer),
		Visibility,
		CaptureSequence,
		AppliedDamage,
		PreviousPhotoHP,
		CurrentPhotoHP,
		GetMaxPhotoHP());

	TrySpawnPhotographedRelic();
	OnGoblinPhotographed.Broadcast(Photographer, Visibility, CaptureSequence);
	BP_OnPhotographed(Photographer, Visibility, CaptureSequence);

	if (PreviousPhotoHP > 0 && CurrentPhotoHP == 0)
	{
		UE_LOG(
			LogNPPhoto,
			Display,
			TEXT("[GoblinPhoto] HP_DEPLETED Goblin=%s Photographer=%s Sequence=%d"),
			*GetNameSafe(this),
			*GetNameSafe(Photographer),
			CaptureSequence);
		BP_OnPhotoHPDepleted(Photographer);
		BeginDespawnPresentation();
	}
}

void ANPGoblinCharacter::TrySpawnPhotographedRelic()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World)
	{
		return;
	}

	if (!PhotographedRelicClass)
	{
		UE_LOG(
			LogNPGoblinCharacter,
			Warning,
			TEXT("촬영 보상 유물 클래스가 설정되지 않았습니다. Goblin=%s"),
			*GetNameSafe(this));
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const FVector SpawnLocation = GetActorLocation()
		+ GetActorTransform().TransformVectorNoScale(PhotographedRelicSpawnOffset);
	ANPBaseRelic* Relic = World->SpawnActor<ANPBaseRelic>(
		PhotographedRelicClass,
		SpawnLocation,
		GetActorRotation(),
		SpawnParameters);
	if (!IsValid(Relic))
	{
		UE_LOG(
			LogNPGoblinCharacter,
			Error,
			TEXT("촬영 보상 유물 생성에 실패했습니다. Goblin=%s RelicClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(PhotographedRelicClass.Get()));
		return;
	}

	Relic->SetReplicates(true);
	Relic->SetReplicateMovement(true);
	const FVector2D HorizontalDirection = FMath::RandPointInCircle(1.0f).GetSafeNormal();
	const FVector LaunchVelocity(
		HorizontalDirection.X * FMath::Max(0.0f, PhotographedRelicHorizontalLaunchSpeed),
		HorizontalDirection.Y * FMath::Max(0.0f, PhotographedRelicHorizontalLaunchSpeed),
		FMath::Max(0.0f, PhotographedRelicUpwardLaunchSpeed));
	const bool bLaunched = Relic->ReleaseWithVelocityImpulse(LaunchVelocity);
	SpawnedPhotoRelic = Relic;
	UE_LOG(
		LogNPGoblinCharacter,
		Display,
		TEXT("고블린 촬영 보상 유물 생성 완료. Goblin=%s Relic=%s Location=%s PhysicsLaunch=%s Velocity=%s"),
		*GetNameSafe(this),
		*GetNameSafe(Relic),
		*Relic->GetActorLocation().ToCompactString(),
		bLaunched ? TEXT("true") : TEXT("false"),
		*LaunchVelocity.ToCompactString());
}
