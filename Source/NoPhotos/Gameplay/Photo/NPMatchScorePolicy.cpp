#include "Gameplay/Photo/NPMatchScorePolicy.h"

#include "Gameplay/Photo/NPPhotoEvidenceTypes.h"

int32 UNPMatchScorePolicy::CalculateEvidenceScore_Implementation(
	const FNPPhotoEvidenceResult& Evidence) const
{
	if (!Evidence.bSuccess)
	{
		return 0;
	}

	const float AverageVisibility = FMath::Clamp(
		(Evidence.ThiefVisibility + Evidence.RelicVisibility) * 0.5f,
		0.0f,
		1.0f);
	return BaseEvidenceScore + FMath::RoundToInt(MaximumVisibilityBonus * AverageVisibility);
}
