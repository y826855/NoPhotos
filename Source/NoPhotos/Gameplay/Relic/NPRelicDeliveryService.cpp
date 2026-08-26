#include "Gameplay/Relic/NPRelicDeliveryService.h"

#include "Core/Main/NPMainGameState.h"
#include "Core/NPPlayerState.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Pawn.h"
#include "Gameplay/Character/Component/NPStablePhysicsGrabComponent.h"
#include "Gameplay/Photo/NPPhotoEvidenceTypes.h"
#include "Gameplay/Relic/NPBaseRelic.h"
#include "Gameplay/Relic/NPRelicReturnZone.h"
#include "NoPhotos.h"
#include "Core/Main/NPMainGameMode.h"

void UNPRelicDeliveryService::Initialize(ANPMainGameMode* InGameMode)
{
	OwningGameMode = InGameMode;
}

UWorld* UNPRelicDeliveryService::GetWorld() const
{
	return OwningGameMode.IsValid() ? OwningGameMode->GetWorld() : nullptr;
}

bool UNPRelicDeliveryService::RegisterPhotoEvidence(const FNPPhotoEvidenceResult& Evidence)
{
	if (!OwningGameMode.IsValid() || !OwningGameMode->HasAuthority() || !Evidence.bSuccess)
	{
		return false;
	}

	ANPBaseRelic* Relic = Cast<ANPBaseRelic>(Evidence.Relic);
	if (!Relic || !IsValid(Evidence.Photographer))
	{
		return false;
	}

	const bool bRegistered = Relic->RegisterEvidencePhotographer(Evidence.Photographer);
	UE_LOG(
		LogNoPhotos,
		Log,
		TEXT("[RelicDelivery] Evidence %s. Relic=%s Photographer=%s UniquePhotographers=%d ReturnScore=%d"),
		bRegistered ? TEXT("registered") : TEXT("ignored"),
		*GetNameSafe(Relic),
		*GetNameSafe(Evidence.Photographer),
		Relic->GetEvidencePhotographerCount(),
		CalculateReturnScore(Relic));
	return bRegistered;
}

int32 UNPRelicDeliveryService::CalculateReturnScore(const ANPBaseRelic* Relic) const
{
	if (!IsValid(Relic) || PhotographersForZeroScore <= 0)
	{
		return 0;
	}

	const int32 PhotographerCount = FMath::Clamp(
		Relic->GetEvidencePhotographerCount(),
		0,
		PhotographersForZeroScore);
	const float RemainingRatio = static_cast<float>(PhotographersForZeroScore - PhotographerCount)
		/ static_cast<float>(PhotographersForZeroScore);
	return FMath::RoundToInt(static_cast<float>(Relic->GetBasePrice()) * RemainingRatio);
}

bool UNPRelicDeliveryService::TryDeliverRelic(
	ANPBaseRelic* Relic,
	ANPRelicReturnZone* ReturnZone)
{
	if (!OwningGameMode.IsValid() || !OwningGameMode->HasAuthority()
		|| !IsValid(Relic) || !IsValid(ReturnZone) || Relic->IsReturned())
	{
		return false;
	}

	ANPMainGameState* MainGameState = GetWorld()
		? GetWorld()->GetGameState<ANPMainGameState>()
		: nullptr;
	ANPPlayerState* Carrier = Cast<ANPPlayerState>(Relic->GetLastCarrierPlayerState());
	if (!MainGameState || !MainGameState->IsMainGameActive() || !Carrier)
	{
		UE_LOG(
			LogNoPhotos,
			Warning,
			TEXT("[RelicDelivery] Delivery rejected. Relic=%s GameActive=%s Carrier=%s"),
			*GetNameSafe(Relic),
			MainGameState && MainGameState->IsMainGameActive() ? TEXT("true") : TEXT("false"),
			*GetNameSafe(Carrier));
		return false;
	}

	const int32 AwardedScore = CalculateReturnScore(Relic);
	if (APawn* CarrierPawn = Carrier->GetPawn())
	{
		if (UNPStablePhysicsGrabComponent* GrabComponent =
			CarrierPawn->FindComponentByClass<UNPStablePhysicsGrabComponent>())
		{
			const UPrimitiveComponent* GrabbedComponent = GrabComponent->GetGrabbedComponent();
			if (GrabbedComponent && GrabbedComponent->GetOwner() == Relic)
			{
				GrabComponent->SetGrabRequested(false);
			}
		}
	}
	if (!Relic->TryMarkReturned())
	{
		return false;
	}

	if (AwardedScore > 0)
	{
		Carrier->AddScore(AwardedScore);
	}
	UE_LOG(
		LogNoPhotos,
		Log,
		TEXT("[RelicDelivery] Relic returned. Relic=%s Carrier=%s BasePrice=%d Photographers=%d AwardedScore=%d"),
		*GetNameSafe(Relic),
		*GetNameSafe(Carrier),
		Relic->GetBasePrice(),
		Relic->GetEvidencePhotographerCount(),
		AwardedScore);
	return true;
}
