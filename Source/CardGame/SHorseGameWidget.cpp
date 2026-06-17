#include "SHorseGameWidget.h"
#include "PokerEval.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Styling/CoreStyle.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include <vector>

#define LOCTEXT_NAMESPACE "HorseGame"

// ----- file-local helpers -----
namespace
{
	std::vector<int> ToStd(const TArray<int32>& A, const TArray<int32>* Extra = nullptr)
	{
		std::vector<int> v;
		v.reserve(A.Num() + (Extra ? Extra->Num() : 0));
		for (int32 c : A) v.push_back((int)c);
		if (Extra) for (int32 c : *Extra) v.push_back((int)c);
		return v;
	}

	// lowval 0=A,1=2,..,12=K  -> display rank
	FString LowCardLabel(int lowval)
	{
		static const TCHAR* L[] = { TEXT("A"), TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"),
			TEXT("6"), TEXT("7"), TEXT("8"), TEXT("9"), TEXT("10"), TEXT("J"), TEXT("Q"), TEXT("K") };
		if (lowval < 0 || lowval > 12) return TEXT("?");
		return L[lowval];
	}

	FString DescribeLow(const poker::EvalResult& R)
	{
		// R.used are 5 cards; sort by low value descending for the conventional "7-5-4-2-A" read.
		TArray<int32> lows;
		for (int c : R.used)
		{
			if (c < 0) continue;
			int r = poker::CardRank(c);
			lows.Add(r == 12 ? 0 : r + 1);
		}
		lows.Sort([](const int32& a, const int32& b) { return a > b; });
		FString s;
		for (int32 i = 0; i < lows.Num(); ++i)
		{
			if (i) s += TEXT("-");
			s += LowCardLabel(lows[i]);
		}
		return s;
	}
}

FString SHorseGameWidget::CardLabel(int32 Card)
{
	if (Card < 0) return TEXT("");
	static const TCHAR* Ranks[] = { TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"), TEXT("6"),
		TEXT("7"), TEXT("8"), TEXT("9"), TEXT("10"), TEXT("J"), TEXT("Q"), TEXT("K"), TEXT("A") };
	static const TCHAR* Suits[] = { TEXT("♠"), TEXT("♥"), TEXT("♦"), TEXT("♣") }; // ♠♥♦♣
	return FString::Printf(TEXT("%s%s"), Ranks[poker::CardRank(Card)], Suits[poker::CardSuit(Card)]);
}

FLinearColor SHorseGameWidget::SuitColor(int32 Card)
{
	const int32 Suit = poker::CardSuit(Card);
	return (Suit == 1 || Suit == 2)
		? FLinearColor(0.85f, 0.12f, 0.12f, 1.f)
		: FLinearColor(0.08f, 0.08f, 0.08f, 1.f);
}

FString SHorseGameWidget::VariantName(int32 V)
{
	switch (V)
	{
	case V_Holdem:  return TEXT("Texas Hold'em");
	case V_Omaha:   return TEXT("Omaha Hi");
	case V_Razz:    return TEXT("Razz");
	case V_Stud:    return TEXT("Seven-card Stud");
	case V_StudHiLo:return TEXT("Stud Hi-Lo (8 or better)");
	}
	return TEXT("?");
}

static FString StreetNameFor(bool bBoard, int32 Street)
{
	if (bBoard)
	{
		static const TCHAR* N[] = { TEXT("Pre-flop"), TEXT("Flop"), TEXT("Turn"), TEXT("River") };
		return (Street >= 0 && Street < 4) ? N[Street] : TEXT("");
	}
	static const TCHAR* N[] = { TEXT("3rd street"), TEXT("4th street"), TEXT("5th street"), TEXT("6th street"), TEXT("7th street") };
	return (Street >= 0 && Street < 5) ? N[Street] : TEXT("");
}

void SHorseGameWidget::Construct(const FArguments& InArgs)
{
	const FSlateFontInfo BannerFont = FCoreStyle::GetDefaultFontStyle("Bold", 40);
	const FSlateFontInfo InfoFont = FCoreStyle::GetDefaultFontStyle("Regular", 30);

	ChildSlot
	[
		SNew(SBorder)
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.05f, 0.32f, 0.16f, 1.f))
		.Padding(FMargin(16))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(4)
			[ SAssignNew(BannerText, STextBlock).Font(BannerFont).ColorAndOpacity(FLinearColor::White) ]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(4)
			[ SAssignNew(InfoText, STextBlock).Font(InfoFont).ColorAndOpacity(FLinearColor(1.f,0.95f,0.7f,1.f)) ]

			// Opponent
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(2)
			[ SNew(STextBlock).Font(InfoFont).ColorAndOpacity(FLinearColor::White).Text(LOCTEXT("Opp","Opponent")) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(6)
			[ SAssignNew(OpponentRow, SHorizontalBox) ]

			// Board
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(10)
			[ SAssignNew(BoardRow, SHorizontalBox) ]

			// Status / result
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(4)
			[ SAssignNew(StatusText, STextBlock).Font(InfoFont).ColorAndOpacity(FLinearColor::White) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(4)
			[ SAssignNew(ResultText, STextBlock).Font(InfoFont).ColorAndOpacity(FLinearColor(0.7f,1.f,0.7f,1.f)) ]

			// Player
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(2)
			[ SNew(STextBlock).Font(InfoFont).ColorAndOpacity(FLinearColor::White).Text(LOCTEXT("You","You")) ]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(6)
			[ SAssignNew(PlayerRow, SHorizontalBox) ]

			// Actions
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(12)
			[ SAssignNew(ActionRow, SHorizontalBox) ]
		]
	];

	LoadAudio();
	NewGame();
}

// ---------------- Audio ----------------

void SHorseGameWidget::LoadAudio()
{
	SfxDeal.Reset(LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/sfx_deal.sfx_deal")));
	SfxChip.Reset(LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/sfx_chip.sfx_chip")));
	SfxCheck.Reset(LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/sfx_check.sfx_check")));
	SfxWin.Reset(LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/sfx_win.sfx_win")));
	SfxLose.Reset(LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/sfx_lose.sfx_lose")));
}

void SHorseGameWidget::PlaySfx(USoundBase* Sound)
{
	if (!Sound) return;
	UWorld* W = (GEngine && GEngine->GameViewport) ? GEngine->GameViewport->GetWorld() : nullptr;
	if (W) UGameplayStatics::PlaySound2D(W, Sound);
}

// ---------------- Flow ----------------

void SHorseGameWidget::NewGame()
{
	PlayerChips = StartChips;
	AIChips = StartChips;
	VariantIndex = V_Holdem;
	bGameOver = false;
	StartHand();
}

void SHorseGameWidget::StartHand()
{
	if (PlayerChips < Ante || AIChips < Ante)
	{
		bGameOver = true;
		bHandActive = false;
		ResultLine = (PlayerChips >= AIChips)
			? TEXT("You busted the opponent! Game over.")
			: TEXT("You're out of chips. Game over.");
		Refresh();
		return;
	}

	Deck.Reset();
	for (int32 i = 0; i < 52; ++i) Deck.Add(i);
	for (int32 i = Deck.Num() - 1; i > 0; --i) Deck.Swap(i, FMath::RandRange(0, i));
	DeckPos = 0;

	PlayerCards.Reset(); AICards.Reset();
	PlayerUp.Reset(); AIUp.Reset();
	Board.Reset();

	Street = 0;
	Pot = 0;
	PlayerCommitted = AICommitted = 0;
	RaisesThisStreet = ActionsThisStreet = 0;
	bHandActive = true;
	bShowdown = false;
	bAllIn = false;
	ResultLine.Empty();

	// Antes
	PlayerChips -= Ante; AIChips -= Ante; Pot = Ante * 2;

	DealInitial();
	PlaySfx(SfxDeal.Get());
	CurrentBetSize = SmallBet;
	BeginStreetBetting();
}

void SHorseGameWidget::DealInitial()
{
	if (IsBoardGame())
	{
		const int32 nHole = (VariantIndex == V_Holdem) ? 2 : 4;
		for (int32 i = 0; i < nHole; ++i)
		{
			PlayerCards.Add(DealCard()); PlayerUp.Add(false);
			AICards.Add(DealCard());     AIUp.Add(false);
		}
		NumStreets = 4; // pre-flop, flop, turn, river
	}
	else
	{
		// Seven-card stud family: two down, one up to start (3rd street)
		for (int32 i = 0; i < 3; ++i)
		{
			const bool up = (i == 2);
			PlayerCards.Add(DealCard()); PlayerUp.Add(up);
			AICards.Add(DealCard());     AIUp.Add(up);
		}
		NumStreets = 5; // 3rd..7th
	}
}

void SHorseGameWidget::BeginStreetBetting()
{
	PlayerCommitted = AICommitted = 0;
	RaisesThisStreet = ActionsThisStreet = 0;
	CurrentBetSize = (Street <= 1) ? SmallBet : BigBet;

	if (bAllIn)
	{
		// No more betting possible; deal the rest and resolve.
		FastForwardToShowdown();
		return;
	}

	bAwaitingPlayer = true;
	StatusLine = FString::Printf(TEXT("%s — your move."), *StreetNameFor(IsBoardGame(), Street));
	Refresh();
}

void SHorseGameWidget::AdvanceStreet()
{
	++Street;
	if (Street >= NumStreets) { Showdown(); return; }

	// Reveal cards for the new street.
	if (IsBoardGame())
	{
		if (Street == 1) { Board.Add(DealCard()); Board.Add(DealCard()); Board.Add(DealCard()); }
		else if (Street == 2) { Board.Add(DealCard()); }
		else if (Street == 3) { Board.Add(DealCard()); }
	}
	else
	{
		const bool up = (Street <= 3); // 4th,5th,6th up; 7th down
		PlayerCards.Add(DealCard()); PlayerUp.Add(up);
		AICards.Add(DealCard());     AIUp.Add(up);
	}

	PlaySfx(SfxDeal.Get());
	BeginStreetBetting();
}

void SHorseGameWidget::FastForwardToShowdown()
{
	// Deal any remaining streets without betting, then showdown.
	while (Street < NumStreets - 1)
	{
		++Street;
		if (IsBoardGame())
		{
			if (Street == 1) { Board.Add(DealCard()); Board.Add(DealCard()); Board.Add(DealCard()); }
			else if (Street == 2) { Board.Add(DealCard()); }
			else if (Street == 3) { Board.Add(DealCard()); }
		}
		else
		{
			const bool up = (Street <= 3);
			PlayerCards.Add(DealCard()); PlayerUp.Add(up);
			AICards.Add(DealCard());     AIUp.Add(up);
		}
	}
	Showdown();
}

int32 SHorseGameWidget::ToCall(bool bPlayer) const
{
	return bPlayer ? FMath::Max(0, AICommitted - PlayerCommitted)
	               : FMath::Max(0, PlayerCommitted - AICommitted);
}

void SHorseGameWidget::ProcessAfterPlayerAction()
{
	if (!bHandActive) return;
	if (RoundComplete()) { AdvanceStreet(); return; }
	AITurn();
}

// ---------------- AI ----------------

static float HighStrengthOf(const std::vector<int>& cs)
{
	if (cs.size() >= 5)
	{
		poker::EvalResult r = poker::BestHigh(cs);
		float s = (float)r.cat / 8.0f + ((float)r.topCard / 12.0f) * 0.12f;
		return FMath::Clamp(s, 0.f, 1.f);
	}
	int cnt[13] = { 0 }; int maxc = 1, hi = 0;
	for (int c : cs) { int r = poker::CardRank(c); cnt[r]++; if (cnt[r] > maxc) maxc = cnt[r]; if (r > hi) hi = r; }
	if (maxc >= 3) return 0.75f;
	if (maxc == 2) return 0.5f;
	return FMath::Clamp(0.15f + ((float)hi / 12.0f) * 0.25f, 0.f, 1.f);
}

static float LowStrengthOf(const std::vector<int>& cs)
{
	if (cs.size() >= 5)
	{
		bool q; poker::EvalResult r = poker::BestLow(cs, q);
		float s = 1.0f - ((float)r.topCard / 12.0f); // lower top card == stronger low
		if (!r.noPair) s *= 0.4f;                     // pairs ruin a low
		return FMath::Clamp(s, 0.f, 1.f);
	}
	int distinctLow = 0; bool seen[13] = { false };
	for (int c : cs)
	{
		int r = poker::CardRank(c); int lv = (r == 12) ? 0 : r + 1;
		if (lv <= 7 && !seen[lv]) { seen[lv] = true; ++distinctLow; }
	}
	return FMath::Clamp((float)distinctLow / 5.0f * 0.7f, 0.f, 1.f);
}

float SHorseGameWidget::AIStrength() const
{
	std::vector<int> a = ToStd(AICards, IsBoardGame() ? &Board : nullptr);
	float s;
	switch (VariantIndex)
	{
	case V_Razz:     s = LowStrengthOf(a); break;
	case V_StudHiLo: s = FMath::Max(HighStrengthOf(a), LowStrengthOf(a)); break;
	default:         s = HighStrengthOf(a); break; // Holdem, Omaha, Stud
	}
	return FMath::Clamp(s + FMath::FRandRange(-0.08f, 0.08f), 0.f, 1.f);
}

void SHorseGameWidget::AITurn()
{
	bAwaitingPlayer = false;
	const float s = AIStrength();
	const int32 toCall = ToCall(false);
	const bool canRaise = RaisesThisStreet < MaxRaises && AIChips > 0;

	auto AIPut = [&](int32 amt)
	{
		amt = FMath::Min(amt, AIChips);
		AIChips -= amt; AICommitted += amt; Pot += amt;
		if (AIChips == 0 || PlayerChips == 0) bAllIn = true;
	};

	if (toCall == 0)
	{
		if (s > 0.55f && canRaise)
		{
			AIPut(CurrentBetSize); ++RaisesThisStreet; ++ActionsThisStreet;
			StatusLine = FString::Printf(TEXT("Opponent bets $%d."), CurrentBetSize);
			PlaySfx(SfxChip.Get());
		}
		else
		{
			++ActionsThisStreet;
			StatusLine = TEXT("Opponent checks.");
			PlaySfx(SfxCheck.Get());
		}
	}
	else
	{
		if (s < 0.30f && toCall >= CurrentBetSize)
		{
			// fold
			StatusLine = TEXT("Opponent folds.");
			++ActionsThisStreet;
			EndHandByFold(false);
			return;
		}
		else if (s > 0.72f && canRaise)
		{
			AIPut(toCall + CurrentBetSize); ++RaisesThisStreet; ++ActionsThisStreet;
			StatusLine = FString::Printf(TEXT("Opponent raises to $%d."), AICommitted);
			PlaySfx(SfxChip.Get());
		}
		else
		{
			AIPut(toCall); ++ActionsThisStreet;
			StatusLine = TEXT("Opponent calls.");
			PlaySfx(SfxChip.Get());
		}
	}

	if (!bHandActive) return;
	if (RoundComplete()) { AdvanceStreet(); return; }
	bAwaitingPlayer = true;
	StatusLine += TEXT("  Your move.");
	Refresh();
}

// ---------------- Showdown / pot ----------------

void SHorseGameWidget::AwardPot(int32 toPlayer, int32 toAI)
{
	PlayerChips += toPlayer;
	AIChips += toAI;
	Pot = 0;
}

void SHorseGameWidget::Showdown()
{
	bShowdown = true;
	bHandActive = false;
	bAwaitingPlayer = false;

	const std::vector<int> p = ToStd(PlayerCards);
	const std::vector<int> a = ToStd(AICards);
	const std::vector<int> board = ToStd(Board);

	const int32 thePot = Pot;
	const int32 chipsBefore = PlayerChips;

	if (VariantIndex == V_Holdem || VariantIndex == V_Stud || VariantIndex == V_Omaha)
	{
		poker::EvalResult pr, ar;
		if (VariantIndex == V_Omaha)
		{
			pr = poker::OmahaHigh(p, board);
			ar = poker::OmahaHigh(a, board);
		}
		else if (VariantIndex == V_Holdem)
		{
			std::vector<int> pc = p; pc.insert(pc.end(), board.begin(), board.end());
			std::vector<int> ac = a; ac.insert(ac.end(), board.begin(), board.end());
			pr = poker::BestHigh(pc); ar = poker::BestHigh(ac);
		}
		else // Stud
		{
			pr = poker::BestHigh(p); ar = poker::BestHigh(a);
		}

		FString pd = poker::HighCatName(pr.cat);
		FString ad = poker::HighCatName(ar.cat);
		if (pr.score > ar.score) { AwardPot(thePot, 0); ResultLine = FString::Printf(TEXT("You win $%d!  You: %s  •  Opp: %s"), thePot, *pd, *ad); }
		else if (pr.score < ar.score) { AwardPot(0, thePot); ResultLine = FString::Printf(TEXT("Opponent wins $%d.  You: %s  •  Opp: %s"), thePot, *pd, *ad); }
		else { int32 h = thePot / 2; AwardPot(thePot - h, h); ResultLine = FString::Printf(TEXT("Split pot ($%d each).  Both: %s"), h, *pd); }
	}
	else if (VariantIndex == V_Razz)
	{
		bool q; poker::EvalResult pr = poker::BestLow(p, q);
		poker::EvalResult ar = poker::BestLow(a, q);
		FString pd = DescribeLow(pr), ad = DescribeLow(ar);
		if (pr.score < ar.score) { AwardPot(thePot, 0); ResultLine = FString::Printf(TEXT("You win $%d!  Low — You: %s  •  Opp: %s"), thePot, *pd, *ad); }
		else if (pr.score > ar.score) { AwardPot(0, thePot); ResultLine = FString::Printf(TEXT("Opponent wins $%d.  Low — You: %s  •  Opp: %s"), thePot, *pd, *ad); }
		else { int32 h = thePot / 2; AwardPot(thePot - h, h); ResultLine = FString::Printf(TEXT("Split pot ($%d each).  Low: %s"), h, *pd); }
	}
	else // Stud Hi-Lo (8 or better)
	{
		poker::EvalResult phi = poker::BestHigh(p), ahi = poker::BestHigh(a);
		bool pq, aq;
		poker::EvalResult plo = poker::BestLow(p, pq), alo = poker::BestLow(a, aq);
		const bool anyLow = pq || aq;

		int32 highPot = (thePot + 1) / 2;   // odd chip goes to the high half
		int32 lowPot = thePot - highPot;
		if (!anyLow) { highPot = thePot; lowPot = 0; }

		int32 pPay = 0, aPay = 0;
		// high half
		if (phi.score > ahi.score) pPay += highPot;
		else if (phi.score < ahi.score) aPay += highPot;
		else { int32 h = highPot / 2; pPay += highPot - h; aPay += h; }
		// low half
		if (lowPot > 0)
		{
			if (pq && (!aq || plo.score < alo.score)) pPay += lowPot;
			else if (aq && (!pq || alo.score < plo.score)) aPay += lowPot;
			else { int32 h = lowPot / 2; pPay += lowPot - h; aPay += h; } // both qualify and tie
		}
		AwardPot(pPay, aPay);

		FString lowStr = anyLow
			? FString::Printf(TEXT("Low — You: %s  Opp: %s"),
				pq ? *DescribeLow(plo) : TEXT("none"),
				aq ? *DescribeLow(alo) : TEXT("none"))
			: TEXT("No qualifying low.");
		ResultLine = FString::Printf(TEXT("You +$%d / Opp +$%d.  High — You: %s  Opp: %s.  %s"),
			pPay, aPay, *FString(poker::HighCatName(phi.cat)), *FString(poker::HighCatName(ahi.cat)), *lowStr);
	}

	PlaySfx(PlayerChips > chipsBefore ? SfxWin.Get() : SfxLose.Get());
	StatusLine = TEXT("Hand complete.");
	Refresh();
}

void SHorseGameWidget::EndHandByFold(bool bPlayerFolded)
{
	bHandActive = false;
	bAwaitingPlayer = false;
	const int32 thePot = Pot;
	if (bPlayerFolded) { AwardPot(0, thePot); ResultLine = FString::Printf(TEXT("You folded. Opponent takes $%d."), thePot); }
	else { AwardPot(thePot, 0); ResultLine = FString::Printf(TEXT("Opponent folds. You take $%d!"), thePot); }
	PlaySfx(bPlayerFolded ? SfxLose.Get() : SfxWin.Get());
	Refresh();
}

// ---------------- Player actions ----------------

FReply SHorseGameWidget::OnFold()
{
	if (!bHandActive || !bAwaitingPlayer) return FReply::Handled();
	EndHandByFold(true);
	return FReply::Handled();
}

FReply SHorseGameWidget::OnCheckCall()
{
	if (!bHandActive || !bAwaitingPlayer) return FReply::Handled();
	const int32 toCall = ToCall(true);
	int32 amt = FMath::Min(toCall, PlayerChips);
	PlayerChips -= amt; PlayerCommitted += amt; Pot += amt;
	if (PlayerChips == 0 || AIChips == 0) bAllIn = true;
	++ActionsThisStreet;
	StatusLine = (toCall == 0) ? TEXT("You check.") : FString::Printf(TEXT("You call $%d."), amt);
	PlaySfx(toCall == 0 ? SfxCheck.Get() : SfxChip.Get());
	bAwaitingPlayer = false;
	ProcessAfterPlayerAction();
	if (bHandActive && bAwaitingPlayer) Refresh();
	return FReply::Handled();
}

FReply SHorseGameWidget::OnBetRaise()
{
	if (!bHandActive || !bAwaitingPlayer) return FReply::Handled();
	if (RaisesThisStreet >= MaxRaises) return FReply::Handled();
	const int32 toCall = ToCall(true);
	int32 amt = FMath::Min(toCall + CurrentBetSize, PlayerChips);
	PlayerChips -= amt; PlayerCommitted += amt; Pot += amt;
	if (PlayerChips == 0 || AIChips == 0) bAllIn = true;
	++RaisesThisStreet; ++ActionsThisStreet;
	StatusLine = (toCall == 0) ? FString::Printf(TEXT("You bet $%d."), amt)
	                           : FString::Printf(TEXT("You raise to $%d."), PlayerCommitted);
	PlaySfx(SfxChip.Get());
	bAwaitingPlayer = false;
	ProcessAfterPlayerAction();
	if (bHandActive && bAwaitingPlayer) Refresh();
	return FReply::Handled();
}

FReply SHorseGameWidget::OnNextHand()
{
	if (bGameOver) return FReply::Handled();
	VariantIndex = (VariantIndex + 1) % 5;
	StartHand();
	return FReply::Handled();
}

FReply SHorseGameWidget::OnNewGame()
{
	NewGame();
	return FReply::Handled();
}

// ---------------- Rendering ----------------

TSharedRef<SWidget> SHorseGameWidget::MakeCardTile(int32 Card, bool bFaceUp) const
{
	const FSlateFontInfo CardFont = FCoreStyle::GetDefaultFontStyle("Bold", 40);
	const bool hidden = !bFaceUp || Card < 0;
	return SNew(SBox).WidthOverride(112).HeightOverride(156)
	[
		SNew(SBorder)
		.HAlign(HAlign_Center).VAlign(VAlign_Center)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(hidden ? FLinearColor(0.15f, 0.2f, 0.55f, 1.f) : FLinearColor::White)
		[
			SNew(STextBlock)
			.Font(CardFont)
			.ColorAndOpacity(FSlateColor(hidden ? FLinearColor(0.8f, 0.85f, 1.f, 1.f) : SuitColor(Card)))
			.Text(FText::FromString(hidden ? TEXT("▒") : CardLabel(Card)))
		]
	];
}

void SHorseGameWidget::FillCardRow(TSharedPtr<SHorizontalBox> Row, const TArray<int32>& Cards,
	const TArray<bool>* Up, bool bRevealAll) const
{
	Row->ClearChildren();
	for (int32 i = 0; i < Cards.Num(); ++i)
	{
		const bool faceUp = bRevealAll || (Up && Up->IsValidIndex(i) && (*Up)[i]);
		Row->AddSlot().AutoWidth().Padding(4)[ MakeCardTile(Cards[i], faceUp) ];
	}
}

void SHorseGameWidget::Refresh()
{
	// Banner: HORSE letters with current variant highlighted.
	{
		const TCHAR* letters[] = { TEXT("H"), TEXT("O"), TEXT("R"), TEXT("S"), TEXT("E") };
		FString banner;
		for (int32 i = 0; i < 5; ++i)
		{
			if (i) banner += TEXT(" ");
			banner += (i == VariantIndex) ? FString::Printf(TEXT("[%s]"), letters[i]) : FString(letters[i]);
		}
		banner += FString::Printf(TEXT("   %s"), *VariantName(VariantIndex));
		BannerText->SetText(FText::FromString(banner));
	}

	InfoText->SetText(FText::FromString(
		FString::Printf(TEXT("Opponent: $%d        Pot: $%d        You: $%d"), AIChips, Pot, PlayerChips)));

	// Cards
	FillCardRow(OpponentRow, AICards, IsBoardGame() ? nullptr : &AIUp, bShowdown);
	FillCardRow(BoardRow, Board, nullptr, true);
	FillCardRow(PlayerRow, PlayerCards, nullptr, true);

	StatusText->SetText(FText::FromString(StatusLine));
	ResultText->SetText(FText::FromString(ResultLine));

	// Action buttons
	ActionRow->ClearChildren();
	const FSlateFontInfo BtnFont = FCoreStyle::GetDefaultFontStyle("Bold", 32);

	auto AddButton = [&](const FString& Label, bool bEnabled, FReply(SHorseGameWidget::*Handler)())
	{
		ActionRow->AddSlot().AutoWidth().Padding(10)
		[
			SNew(SButton)
			.HAlign(HAlign_Center).VAlign(VAlign_Center)
			.ContentPadding(FMargin(40, 22))
			.IsEnabled(bEnabled)
			.OnClicked(this, Handler)
			[ SNew(STextBlock).Font(BtnFont).Text(FText::FromString(Label)) ]
		];
	};

	if (bGameOver)
	{
		AddButton(TEXT("New Game"), true, &SHorseGameWidget::OnNewGame);
	}
	else if (!bHandActive)
	{
		AddButton(TEXT("Next Hand"), true, &SHorseGameWidget::OnNextHand);
	}
	else
	{
		const int32 toCall = ToCall(true);
		const bool turn = bAwaitingPlayer;
		const bool canBetRaise = turn && RaisesThisStreet < MaxRaises && PlayerChips > 0;

		AddButton(TEXT("Fold"), turn && toCall > 0, &SHorseGameWidget::OnFold);
		AddButton(toCall == 0 ? TEXT("Check") : FString::Printf(TEXT("Call $%d"), FMath::Min(toCall, PlayerChips)),
			turn, &SHorseGameWidget::OnCheckCall);
		AddButton(toCall == 0 ? FString::Printf(TEXT("Bet $%d"), CurrentBetSize)
			: FString::Printf(TEXT("Raise $%d"), CurrentBetSize),
			canBetRaise, &SHorseGameWidget::OnBetRaise);
	}
}

#undef LOCTEXT_NAMESPACE
