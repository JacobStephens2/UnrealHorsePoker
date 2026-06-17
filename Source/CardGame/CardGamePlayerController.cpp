#include "CardGamePlayerController.h"
#include "SHorseGameWidget.h"
#include "Widgets/SWeakWidget.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ACardGamePlayerController::ACardGamePlayerController()
{
	bShowMouseCursor = true;
}

void ACardGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (GEngine && GEngine->GameViewport)
	{
		GameWidget = SNew(SHorseGameWidget);

		GEngine->GameViewport->AddViewportWidgetContent(
			SNew(SWeakWidget).PossiblyNullContent(GameWidget.ToSharedRef()));

		// Touch / mouse driven UI.
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GameWidget);
		SetInputMode(InputMode);
		bShowMouseCursor = true;

		// Looping background music. SpawnSound2D returns an AudioComponent we keep
		// referenced (via the UPROPERTY) so the looping sound isn't orphaned.
		if (USoundBase* Music = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/poker_theme.poker_theme")))
		{
			MusicComponent = UGameplayStatics::SpawnSound2D(this, Music, 0.5f);
		}
	}
}
