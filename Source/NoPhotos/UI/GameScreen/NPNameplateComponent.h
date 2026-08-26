#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "NPNameplateComponent.generated.h"

class APlayerState;
class UNPUserNameWidget;

/**
 * 플레이어 머리 위에 표시되는 이름표 위젯입니다.
 * 이 컴포넌트 하나가 위젯 표시, PlayerState 연결, 로컬 카메라 추적을 담당합니다.
 */
UCLASS(Blueprintable, ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class NOPHOTOS_API UNPNameplateComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UNPNameplateComponent();

	/** PlayerState와 이름표 위젯 연결을 다시 시도합니다. */
	UFUNCTION(BlueprintCallable, Category="Nameplate")
	bool RefreshNameplate();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateFacingCamera();

	UPROPERTY(Transient)
	TObjectPtr<UNPUserNameWidget> NameplateWidget;

	UPROPERTY(Transient)
	TObjectPtr<APlayerState> BoundPlayerState;
};
