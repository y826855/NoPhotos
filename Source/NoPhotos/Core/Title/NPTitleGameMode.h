// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NPTitleGameMode.generated.h"

/**
 * 
 */
UCLASS()
class NOPHOTOS_API ANPTitleGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ANPTitleGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
};
