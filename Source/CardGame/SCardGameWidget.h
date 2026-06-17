#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class STextBlock;
class SHorizontalBox;

/**
 * Pure-Slate 2D card game UI: "Higher or Lower".
 * - You hold a hand of cards. A face-up "table" card is shown.
 * - Tap a card in your hand. If its rank is higher than the table card you score a point.
 * - The played card becomes the new table card and a replacement is drawn from the deck.
 * - When the deck and hand are exhausted the round ends; tap "New Game" to replay.
 */
class SCardGameWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCardGameWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	// --- Game state ---
	TArray<int32> Deck;       // remaining cards (0..51)
	TArray<int32> Hand;       // up to HandSize cards; -1 == empty slot
	int32 TableCard = -1;     // current face-up card on the table
	int32 Score = 0;
	int32 Plays = 0;
	bool bGameOver = false;

	static constexpr int32 HandSize = 5;

	// --- Widget refs we refresh on each play ---
	TSharedPtr<STextBlock> ScoreText;
	TSharedPtr<STextBlock> TableText;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<SHorizontalBox> HandBox;

	// --- Logic ---
	void NewGame();
	void DrawIntoSlot(int32 Slot);
	FReply OnCardClicked(int32 Slot);
	FReply OnNewGameClicked();
	void Refresh();

	// --- Card helpers ---
	static int32 RankOf(int32 Card) { return Card % 13; }   // 0..12  (2..A)
	static int32 SuitOf(int32 Card) { return Card / 13; }   // 0..3
	static FString CardLabel(int32 Card);
	static FLinearColor SuitColor(int32 Card);
};
