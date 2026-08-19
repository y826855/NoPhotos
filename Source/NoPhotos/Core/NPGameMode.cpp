#include "NPGameMode.h"

#include "NPGameState.h"

ANPGameMode::ANPGameMode()
{
	GameStateClass = ANPGameState::StaticClass();
}
