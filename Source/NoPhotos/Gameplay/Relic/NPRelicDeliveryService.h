#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NPRelicDeliveryService.generated.h"

class ANPBaseRelic;
class ANoPhotosGameMode;
class ANPRelicReturnZone;
struct FNPPhotoEvidenceResult;

/** 서버에서 Relic의 증거 누적, 반환 가치 계산과 최종 점수 지급을 처리합니다. */
UCLASS()
class NOPHOTOS_API UNPRelicDeliveryService : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(ANoPhotosGameMode* InGameMode);
	virtual UWorld* GetWorld() const override;

	bool RegisterPhotoEvidence(const FNPPhotoEvidenceResult& Evidence);
	bool TryDeliverRelic(ANPBaseRelic* Relic, ANPRelicReturnZone* ReturnZone);

	UFUNCTION(BlueprintPure, Category="Relic|Delivery")
	int32 CalculateReturnScore(const ANPBaseRelic* Relic) const;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<ANoPhotosGameMode> OwningGameMode;

	/** 이 수만큼 서로 다른 플레이어에게 촬영되면 반환 점수가 0이 됩니다. */
	UPROPERTY(EditDefaultsOnly, Category="Relic|Delivery", meta=(ClampMin="1"))
	int32 PhotographersForZeroScore = 3;
};
