#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NPRelicData.generated.h"

UENUM(BlueprintType)
enum class ENPRelicGrade : uint8
{
	Cheap UMETA(DisplayName="Cheap"),
	Rare UMETA(DisplayName="Rare"),
	Precious UMETA(DisplayName="Precious")
};

USTRUCT(BlueprintType)
struct NOPHOTOS_API FNPRelicData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Relic")
	FName RelicID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Relic")
	ENPRelicGrade Grade = ENPRelicGrade::Cheap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Relic", meta=(ClampMin="0"))
	int32 Price = 0;
};
