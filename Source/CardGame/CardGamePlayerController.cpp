#include "CardGamePlayerController.h"
#include "SCardGameWidget.h"
#include "Widgets/SWeakWidget.h"
#include "Engine/GameViewportClient.h"

ACardGamePlayerController::ACardGamePlayerController()
{
	bShowMouseCursor = true;
}

void ACardGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine && GEngine->GameViewport)
	{
		GameWidget = SNew(SCardGameWidget);

		GEngine->GameViewport->AddViewportWidgetContent(
			SNew(SWeakWidget).PossiblyNullContent(GameWidget.ToSharedRef()));

		// Touch / mouse driven UI.
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GameWidget);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
}
