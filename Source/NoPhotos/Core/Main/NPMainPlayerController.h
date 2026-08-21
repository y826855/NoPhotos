#pragma once

#include "CoreMinimal.h"
#include "NoPhotosPlayerController.h"
#include "NPMainPlayerController.generated.h"

class UNPUserWidget;

UCLASS()
class NOPHOTOS_API ANPMainPlayerController	: public ANoPhotosPlayerController
{
	GENERATED_BODY()

public:
	ANPMainPlayerController();
	
	UFUNCTION(BlueprintPure, Category = "Room")
	bool IsListenServerHost() const;
	UFUNCTION(BlueprintCallable, Category = "Room")
	void RequestRestartRoom();
	UFUNCTION(BlueprintCallable, Category = "Room")
	void ExitToMainMenu();
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowGameScreenUI();
	UFUNCTION(Client, Reliable)
	void ClientShowGameScreenUI();
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowSelectPictureUI();
	UFUNCTION(Client, Reliable)
	void ClientShowSelectPictureUI();
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowResultUI();
	UFUNCTION(Client, Reliable)
	void ClientShowResultUI();

	//서버가 선택 사진과 완료 상태를 확인
	UFUNCTION(Server, Reliable)
	void ServerConfirmPictureSelection(const TArray<FGuid>& SelectedPhotoIds);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPUserWidget> GameScreenWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPUserWidget> SelectPictureWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UNPUserWidget> ResultWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Room")
	TSoftObjectPtr<UWorld> MainMenuLevel;

private:
	UFUNCTION(Server, Reliable)
	void ServerRequestRestartRoom();

	void ShowSingleScreen(
		TSubclassOf<UNPUserWidget> WidgetClass);
};