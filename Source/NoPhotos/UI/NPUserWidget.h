#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NPUserWidget.generated.h"

UCLASS()
class NOPHOTOS_API UNPUserWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// UIManager가 이 위젯을 화면 스택에 추가한 직후 호출한다.
	UFUNCTION(BlueprintNativeEvent, Category = "UI")
	void OnPushed();

	// UIManager가 이 위젯에 닫기 요청을 보낼 때 호출한다.
	// 기본 구현은 즉시 닫지만, WBP에서는 닫기 애니메이션 후 CompletePop을 호출할 수 있다.
	UFUNCTION(BlueprintNativeEvent, Category = "UI")
	void OnPopRequested();

	// 위젯이 스택과 Viewport에서 완전히 제거되기 직전에 호출한다.
	UFUNCTION(BlueprintNativeEvent, Category = "UI")
	void OnPopped();

	// 현재 위젯의 닫기 처리를 완료하고 UIManager에 실제 제거를 요청한다.
	UFUNCTION(BlueprintCallable, Category = "UI")
	bool CompletePop();

	// 현재 이 위젯이 닫기 요청을 받은 상태인지 반환한다.
	bool IsPopRequested() const { return bIsPopRequested; }
	// UIManager가 닫기 요청 상태를 설정한다.
	void SetPopRequested(bool bInPopRequested) { bIsPopRequested = bInPopRequested; }
	// 이 위젯이 열려 있는 동안 게임 입력과 UI 입력을 함께 사용할지 반환한다.
	bool UsesGameAndUIInputMode() const { return bUseGameAndUIInputMode; }
	// 이 위젯이 열려 있는 동안 마우스 커서를 표시할지 반환한다.
	bool ShouldShowMouseCursor() const { return bShowMouseCursor; }

private:
	// HUD처럼 게임 입력을 유지해야 하는 화면은 WBP 기본값에서 true로 설정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Input", meta = (AllowPrivateAccess = "true"))
	bool bUseGameAndUIInputMode = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Input", meta = (AllowPrivateAccess = "true"))
	bool bShowMouseCursor = true;

	UPROPERTY(Transient)
	bool bIsPopRequested = false;
};
