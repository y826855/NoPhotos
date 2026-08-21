#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPRelicReturnZone.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

/** 레벨에 배치하여 서버에서 Relic 반환 Overlap을 감지하는 구역입니다. */
UCLASS(Blueprintable)
class NOPHOTOS_API ANPRelicReturnZone : public AActor
{
	GENERATED_BODY()

public:
	ANPRelicReturnZone();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleReturnVolumeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> ReturnVolume;
};
