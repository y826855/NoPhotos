// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/Title/NPTitleGameMode.h"

#include "Core/Title/NPTitleGameState.h"
#include "Core/Title/NPTitlePlayerController.h"

ANPTitleGameMode::ANPTitleGameMode()
{
	GameStateClass = ANPTitleGameState::StaticClass();
	PlayerControllerClass = ANPTitlePlayerController::StaticClass();
}

void ANPTitleGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (ANPTitlePlayerController* TitlePlayerController = Cast<ANPTitlePlayerController>(NewPlayer))
	{
		TitlePlayerController->ClientShowMainMenuUI();
	}
}

