#include "CardGameGameModeBase.h"
#include "CardGamePlayerController.h"
#include "GameFramework/DefaultPawn.h"

ACardGameGameModeBase::ACardGameGameModeBase()
{
	PlayerControllerClass = ACardGamePlayerController::StaticClass();
	DefaultPawnClass = ADefaultPawn::StaticClass();
}
