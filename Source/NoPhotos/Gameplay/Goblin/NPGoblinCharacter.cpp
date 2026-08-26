#include "NPGoblinCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "NPGoblinAIController.h"

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

bool ANPGoblinCharacter::CanBePhotographed_Implementation(APlayerState* Photographer) const
{
	return HasAuthority()
		&& IsValid(Photographer)
		&& !PhotographedPlayers.Contains(Photographer);
}

void ANPGoblinCharacter::OnPhotographed_Implementation(
	APlayerState* Photographer,
	const float Visibility,
	const int32 CaptureSequence)
{
	if (!HasAuthority()
		|| !IsValid(Photographer)
		|| PhotographedPlayers.Contains(Photographer))
	{
		return;
	}

	PhotographedPlayers.Add(Photographer);
	OnGoblinPhotographed.Broadcast(Photographer, Visibility, CaptureSequence);
	BP_OnPhotographed(Photographer, Visibility, CaptureSequence);
}
