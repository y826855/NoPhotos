#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NPUserWidget.generated.h"

UENUM(BlueprintType)
enum class ENPWidgetInputMode : uint8
{
	UIOnly UMETA(DisplayName = "UI Only"),
	GameAndUI UMETA(DisplayName = "Game and UI"),
	GameOnly UMETA(DisplayName = "Game Only")
};

UCLASS()
class NOPHOTOS_API UNPUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintNativeEvent, Category = "UI")
	void OnPushed();
	UFUNCTION(BlueprintNativeEvent, Category = "UI")
	void OnPopRequested();
	UFUNCTION(BlueprintNativeEvent, Category = "UI")
	void OnPopped();
	// 현재 위젯의 닫기 처리를 완료하고 UIManager에 실제 제거를 요청
	UFUNCTION(BlueprintCallable, Category = "UI")
	bool CompletePop();
	// 현재 이 위젯이 닫기 요청을 받은 상태인지 반환한다.
	bool IsPopRequested() const { return bIsPopRequested; }
	// UIManager가 닫기 요청 상태를 설정한다.
	void SetPopRequested(bool bInPopRequested) { bIsPopRequested = bInPopRequested; }
	
	//게임 입력과 UI 입력 동시 사용
	bool UsesGameAndUIInputMode() const { return bUseGameAndUIInputMode; }
	//게임 입력만 사용
	bool UsesGameOnlyInputMode() const { return bUseGameOnlyInputMode; }
	//마우스 커서 표시 여부 확인
	bool ShouldShowMouseCursor() const { return bShowMouseCursor; }

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Input", meta = (AllowPrivateAccess = "true"))
	bool bUseGameAndUIInputMode = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Input", meta = (AllowPrivateAccess = "true"))
	bool bUseGameOnlyInputMode = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Input", meta = (AllowPrivateAccess = "true"))
	bool bShowMouseCursor = true;

	UPROPERTY(Transient)
	bool bIsPopRequested = false;
};
