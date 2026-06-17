#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "UObject/StrongObjectPtr.h"

class STextBlock;
class SHorizontalBox;
class SVerticalBox;
class USoundBase;

/**
 * HORSE poker, heads-up vs a simple AI, with fixed-limit betting.
 * Rotates each hand through the five variants:
 *   H - Texas Hold'em      (2 hole + 5 board, high)
 *   O - Omaha (Hi)         (4 hole, use exactly 2 + 3 board, high)
 *   R - Razz               (7-card stud, A-5 lowball)
 *   S - Seven-card Stud    (7 cards, high)
 *   E - Stud Hi-Lo (8 o/b) (7-card stud, pot split high / qualifying low)
 */
class SHorseGameWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SHorseGameWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	enum EVariant { V_Holdem = 0, V_Omaha, V_Razz, V_Stud, V_StudHiLo };

	// ---- Persistent / table state ----
	int32 VariantIndex = 0;
	int32 PlayerChips = 0;
	int32 AIChips = 0;
	bool bGameOver = false;

	// ---- Per-hand state ----
	TArray<int32> Deck;
	int32 DeckPos = 0;
	TArray<int32> PlayerCards;
	TArray<int32> AICards;
	TArray<bool> PlayerUp; // stud face-up flags (aligned with PlayerCards)
	TArray<bool> AIUp;
	TArray<int32> Board;   // community cards (board games)

	int32 Street = 0;
	int32 NumStreets = 0;
	int32 Pot = 0;
	int32 PlayerCommitted = 0; // chips put in this street
	int32 AICommitted = 0;
	int32 CurrentBetSize = 0;  // limit size for this street
	int32 RaisesThisStreet = 0;
	int32 ActionsThisStreet = 0;

	bool bHandActive = false;   // a hand is in progress
	bool bAwaitingPlayer = false;
	bool bShowdown = false;     // reveal opponent cards
	bool bAllIn = false;
	FString StatusLine;
	FString ResultLine;

	// Tunables
	static constexpr int32 StartChips = 1000;
	static constexpr int32 Ante = 5;
	static constexpr int32 SmallBet = 20;
	static constexpr int32 BigBet = 40;
	static constexpr int32 MaxRaises = 4;

	// ---- Widget refs (rebuilt each Refresh) ----
	TSharedPtr<STextBlock> BannerText;
	TSharedPtr<STextBlock> InfoText;
	TSharedPtr<STextBlock> StatusText;
	TSharedPtr<STextBlock> ResultText;
	TSharedPtr<SHorizontalBox> OpponentRow;
	TSharedPtr<SHorizontalBox> BoardRow;
	TSharedPtr<SHorizontalBox> PlayerRow;
	TSharedPtr<SHorizontalBox> ActionRow;

	// ---- Flow ----
	void NewGame();
	void StartHand();
	void DealInitial();
	void BeginStreetBetting();
	void AdvanceStreet();
	void AITurn();
	void ProcessAfterPlayerAction();
	void Showdown();
	void EndHandByFold(bool bPlayerFolded);
	void AwardPot(int32 toPlayer, int32 toAI);
	void FastForwardToShowdown();

	int32 DealCard() { return Deck[DeckPos++]; }
	bool IsBoardGame() const { return VariantIndex == V_Holdem || VariantIndex == V_Omaha; }
	int32 ToCall(bool bPlayer) const;
	bool RoundComplete() const { return ActionsThisStreet >= 2 && PlayerCommitted == AICommitted; }

	// ---- AI ----
	float AIStrength() const;

	// ---- Player actions ----
	FReply OnFold();
	FReply OnCheckCall();
	FReply OnBetRaise();
	FReply OnNextHand();
	FReply OnNewGame();

	// ---- Rendering ----
	void Refresh();
	TSharedRef<class SWidget> MakeCardTile(int32 Card, bool bFaceUp) const;
	void FillCardRow(TSharedPtr<SHorizontalBox> Row, const TArray<int32>& Cards,
		const TArray<bool>* Up, bool bRevealAll) const;

	// ---- Audio ----
	TStrongObjectPtr<USoundBase> SfxDeal;
	TStrongObjectPtr<USoundBase> SfxChip;
	TStrongObjectPtr<USoundBase> SfxCheck;
	TStrongObjectPtr<USoundBase> SfxWin;
	TStrongObjectPtr<USoundBase> SfxLose;
	void LoadAudio();
	void PlaySfx(USoundBase* Sound);

	// ---- Card / hand helpers ----
	static FString CardLabel(int32 Card);
	static FLinearColor SuitColor(int32 Card);
	static FString VariantName(int32 V);
};
