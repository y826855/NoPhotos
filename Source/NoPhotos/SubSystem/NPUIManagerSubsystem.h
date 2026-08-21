#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NPUIManagerSubsystem.generated.h"

UCLASS()
class NOPHOTOS_API UNPUIManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UNPUIManagerSubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "UI")
	UNPUserWidget* PushWidget(TSubclassOf<UNPUserWidget> WidgetClass, int32 ZOrder = 0);
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	bool RequestPopWidget();

	UFUNCTION(BlueprintCallable, Category = "UI")
	bool CompletePopWidget(UNPUserWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void PopAllWidgets();

	UFUNCTION(BlueprintPure, Category = "UI")
	UNPUserWidget* GetTopWidget() const;
	
private:
	// 현재 클라이언트의 로컬 PlayerController를 찾아 반환. 전용 서버에서는 nullptr을 반환.
	class APlayerController* GetOwningPlayerController() const;
	// 최상단 위젯의 설정에 맞춰 로컬 플레이어의 입력 모드와 마우스 커서를 갱신한다.
	void ApplyInputModeForTopWidget();
	bool RemoveWidgetFromStack(UNPUserWidget* Widget, bool bForceRemove);
	void CleanInvalidWidgetsFromStack();
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNPUserWidget>> WidgetStack;
};
