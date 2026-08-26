#include "NPMapEventLocationCollector.h"

#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "NPMapEventManager.h"
#include "NPMapEventSpawnPoint.h"
#include "NPMapEventSpawnVolume.h"

DEFINE_LOG_CATEGORY_STATIC(LogNPMapEventLocationCollector, Log, All);

ANPMapEventLocationCollector::ANPMapEventLocationCollector()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}

bool ANPMapEventLocationCollector::SupportsSpawnGroup(const FGameplayTag SpawnGroup) const
{
	return SpawnGroup.IsValid() && DefaultSpawnGroups.HasTagExact(SpawnGroup);
}

void ANPMapEventLocationCollector::BeginPlay()
{
	Super::BeginPlay();
	RefreshLocations();

	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (UNPMapEventManagerComponent* EventManager = GameState
		? GameState->FindComponentByClass<UNPMapEventManagerComponent>()
		: nullptr)
	{
		EventManager->RegisterLocationCollector(this);
	}
}

void ANPMapEventLocationCollector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (UNPMapEventManagerComponent* EventManager = GameState
		? GameState->FindComponentByClass<UNPMapEventManagerComponent>()
		: nullptr)
	{
		EventManager->UnregisterLocationCollector(this);
	}

	CollectedSpawnPoints.Reset();
	CollectedSpawnVolumes.Reset();
	Super::EndPlay(EndPlayReason);
}

void ANPMapEventLocationCollector::RefreshLocations(
	const ENPMapEventLocationSource LocationSource)
{
	CollectedSpawnPoints.Reset();
	CollectedSpawnVolumes.Reset();
	LastCollectionSource = LocationSource;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (LocationSource != ENPMapEventLocationSource::Volume)
	{
		for (TActorIterator<ANPMapEventSpawnPoint> Iterator(World); Iterator; ++Iterator)
		{
			if (ANPMapEventSpawnPoint* SpawnPoint = *Iterator;
				IsValid(SpawnPoint) && SpawnPoint->GetLevel() == GetLevel())
			{
				CollectedSpawnPoints.Add(SpawnPoint);
			}
		}
	}

	if (LocationSource != ENPMapEventLocationSource::Point)
	{
		for (TActorIterator<ANPMapEventSpawnVolume> Iterator(World); Iterator; ++Iterator)
		{
			if (ANPMapEventSpawnVolume* SpawnVolume = *Iterator;
				IsValid(SpawnVolume) && SpawnVolume->GetLevel() == GetLevel())
			{
				CollectedSpawnVolumes.Add(SpawnVolume);

				const FVector Location = SpawnVolume->GetActorLocation();
				const FRotator Rotation = SpawnVolume->GetActorRotation();
				const FVector Scale = SpawnVolume->GetActorScale3D();
				const FBox WorldBounds = SpawnVolume->GetComponentsBoundingBox(true);
				UE_LOG(
					LogNPMapEventLocationCollector,
					Display,
					TEXT("Level Instance SpawnVolume 수집: Collector=%s, Level=%s, Volume=%s, Location=%s, Rotation=%s, Scale=%s, BoundsCenter=%s, BoundsExtent=%s"),
					*GetNameSafe(this),
					*GetNameSafe(GetLevel()),
					*GetNameSafe(SpawnVolume),
					*Location.ToCompactString(),
					*Rotation.ToCompactString(),
					*Scale.ToCompactString(),
					*WorldBounds.GetCenter().ToCompactString(),
					*WorldBounds.GetExtent().ToCompactString());
			}
		}
	}

	UE_LOG(
		LogNPMapEventLocationCollector,
		Display,
		TEXT("Level Instance 위치 수집 완료: Collector=%s, Level=%s, Source=%s, SpawnPoints=%d, SpawnVolumes=%d"),
		*GetNameSafe(this),
		*GetNameSafe(GetLevel()),
		*StaticEnum<ENPMapEventLocationSource>()->GetNameStringByValue(static_cast<int64>(LocationSource)),
		CollectedSpawnPoints.Num(),
		CollectedSpawnVolumes.Num());
}

void ANPMapEventLocationCollector::GetSpawnPointsForGroup(
	const FGameplayTag SpawnGroup,
	TArray<ANPMapEventSpawnPoint*>& OutSpawnPoints) const
{
	OutSpawnPoints.Reset();
	const bool bCollectorSupportsGroup = SupportsSpawnGroup(SpawnGroup);
	for (ANPMapEventSpawnPoint* SpawnPoint : CollectedSpawnPoints)
	{
		if (IsValid(SpawnPoint)
			&& SpawnPoint->IsSpawnEnabled()
			&& (SpawnPoint->SupportsSpawnGroup(SpawnGroup)
				|| (SpawnPoint->UsesCollectorSpawnGroups() && bCollectorSupportsGroup))
			&& SpawnPoint->GetSelectionWeight() > 0.0f)
		{
			OutSpawnPoints.Add(SpawnPoint);
		}
	}
}

void ANPMapEventLocationCollector::GetSpawnVolumesForGroup(
	const FGameplayTag SpawnGroup,
	TArray<ANPMapEventSpawnVolume*>& OutSpawnVolumes) const
{
	OutSpawnVolumes.Reset();
	const bool bCollectorSupportsGroup = SupportsSpawnGroup(SpawnGroup);
	for (ANPMapEventSpawnVolume* SpawnVolume : CollectedSpawnVolumes)
	{
		if (IsValid(SpawnVolume)
			&& (SpawnVolume->SupportsSpawnGroup(SpawnGroup)
				|| (SpawnVolume->UsesCollectorSpawnGroups() && bCollectorSupportsGroup))
			&& SpawnVolume->GetSelectionWeight() > 0.0f)
		{
			OutSpawnVolumes.Add(SpawnVolume);
		}
	}
}

ANPMapEventSpawnPoint* ANPMapEventLocationCollector::FindRandomSpawnPoint(
	const FGameplayTag SpawnGroup) const
{
	TArray<ANPMapEventSpawnPoint*> Candidates;
	GetSpawnPointsForGroup(SpawnGroup, Candidates);

	float TotalWeight = 0.0f;
	for (const ANPMapEventSpawnPoint* Candidate : Candidates)
	{
		TotalWeight += Candidate->GetSelectionWeight();
	}

	if (Candidates.IsEmpty() || TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	float Selection = FMath::FRandRange(0.0f, TotalWeight);
	for (ANPMapEventSpawnPoint* Candidate : Candidates)
	{
		Selection -= Candidate->GetSelectionWeight();
		if (Selection <= 0.0f)
		{
			return Candidate;
		}
	}

	return Candidates.Last();
}

bool ANPMapEventLocationCollector::FindRandomSpawnTransformInVolume(
	const FGameplayTag SpawnGroup,
	const FVector RequiredHalfExtent,
	FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;
	TArray<ANPMapEventSpawnVolume*> Candidates;
	GetSpawnVolumesForGroup(SpawnGroup, Candidates);

	float TotalWeight = 0.0f;
	for (const ANPMapEventSpawnVolume* Candidate : Candidates)
	{
		TotalWeight += Candidate->GetSelectionWeight();
	}

	while (!Candidates.IsEmpty() && TotalWeight > 0.0f)
	{
		float Selection = FMath::FRandRange(0.0f, TotalWeight);
		int32 SelectedIndex = Candidates.Num() - 1;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			Selection -= Candidates[Index]->GetSelectionWeight();
			if (Selection <= 0.0f)
			{
				SelectedIndex = Index;
				break;
			}
		}

		ANPMapEventSpawnVolume* SelectedVolume = Candidates[SelectedIndex];
		if (SelectedVolume->FindRandomGroundTransform(RequiredHalfExtent, OutTransform))
		{
			return true;
		}

		TotalWeight -= SelectedVolume->GetSelectionWeight();
		Candidates.RemoveAtSwap(SelectedIndex, 1, EAllowShrinking::No);
	}

	return false;
}
