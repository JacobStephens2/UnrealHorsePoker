#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CardGamePlayerController.generated.h"

UCLASS()
class ACardGamePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACardGamePlayerController();

protected:
	virtual void BeginPlay() override;

private:
	TSharedPtr<class SHorseGameWidget> GameWidget;
};
