#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NPPhotoFlashWidget.generated.h"

/** 사진 촬영 직후 로컬 화면에 표시되는 플래시 효과의 C++ 부모 위젯입니다. */
UCLASS(Abstract)
class NOPHOTOS_API UNPPhotoFlashWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 위젯을 표시하고 WBP의 플래시 애니메이션 이벤트를 실행합니다. */
	UFUNCTION(BlueprintCallable, Category="Photo|Flash")
	void PlayFlash();

protected:
	virtual void NativeDestruct() override;

	/** WBP에서 구현하여 흰 화면 유지 및 페이드아웃 애니메이션을 재생합니다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Photo|Flash", meta=(DisplayName="Play Photo Flash Animation"))
	void BP_PlayFlashAnimation();

	/** 애니메이션 실행 후 위젯을 자동으로 숨길 때까지의 시간입니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Photo|Flash", meta=(ClampMin="0.0"))
	float FlashDisplayDuration = 0.2f;

private:
	void FinishFlash();

	FTimerHandle FlashHideTimer;
};
