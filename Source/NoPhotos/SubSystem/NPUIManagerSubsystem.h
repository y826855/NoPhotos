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

	//현재 입력 설정 다시 적용
	void RefreshTopWidgetInputMode();
	
private:
	class APlayerController* GetOwningPlayerController() const;
	void ApplyInputModeForTopWidget();
	bool RemoveWidgetFromStack(UNPUserWidget* Widget, bool bForceRemove);
	void CleanInvalidWidgetsFromStack();
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNPUserWidget>> WidgetStack;
};
