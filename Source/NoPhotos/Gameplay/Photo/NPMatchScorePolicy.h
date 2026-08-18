#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Photo/NPPhotoEvidenceTypes.h"
#include "UObject/Object.h"
#include "NPMatchScorePolicy.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class NOPHOTOS_API UNPMatchScorePolicy : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category="Photo|Score")
	int32 CalculateEvidenceScore(const FNPPhotoEvidenceResult& Evidence) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Photo|Score", meta=(ClampMin="0"))
	int32 BaseEvidenceScore = 100;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Photo|Score", meta=(ClampMin="0"))
	int32 MaximumVisibilityBonus = 50;
};
