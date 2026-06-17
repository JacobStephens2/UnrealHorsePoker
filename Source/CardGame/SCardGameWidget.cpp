#include "SCardGameWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/CoreStyle.h"
#include "Math/UnrealMathUtility.h"

#define LOCTEXT_NAMESPACE "CardGame"

FString SCardGameWidget::CardLabel(int32 Card)
{
	if (Card < 0)
	{
		return TEXT("");
	}

	static const TCHAR* Ranks[] = { TEXT("2"), TEXT("3"), TEXT("4"), TEXT("5"), TEXT("6"),
		TEXT("7"), TEXT("8"), TEXT("9"), TEXT("10"), TEXT("J"), TEXT("Q"), TEXT("K"), TEXT("A") };
	// Spades, Hearts, Diamonds, Clubs
	static const TCHAR* Suits[] = { TEXT("♠"), TEXT("♥"), TEXT("♦"), TEXT("♣") };

	return FString::Printf(TEXT("%s%s"), Ranks[RankOf(Card)], Suits[SuitOf(Card)]);
}

FLinearColor SCardGameWidget::SuitColor(int32 Card)
{
	// Hearts (1) and Diamonds (2) are red; Spades (0) and Clubs (3) are dark.
	const int32 Suit = SuitOf(Card);
	return (Suit == 1 || Suit == 2)
		? FLinearColor(0.85f, 0.12f, 0.12f, 1.f)
		: FLinearColor(0.08f, 0.08f, 0.08f, 1.f);
}

void SCardGameWidget::Construct(const FArguments& InArgs)
{
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 36);
	const FSlateFontInfo InfoFont  = FCoreStyle::GetDefaultFontStyle("Regular", 24);

	ChildSlot
	[
		SNew(SBorder)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.05f, 0.35f, 0.15f, 1.f)) // card-table green
		[
			SNew(SVerticalBox)

			// Title
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(10)
			[
				SNew(STextBlock)
				.Font(TitleFont)
				.ColorAndOpacity(FLinearColor::White)
				.Text(LOCTEXT("Title", "Higher or Lower"))
			]

			// Score
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(6)
			[
				SAssignNew(ScoreText, STextBlock)
				.Font(InfoFont)
				.ColorAndOpacity(FLinearColor::White)
			]

			// Table card
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(20)
			[
				SNew(SBox).WidthOverride(160).HeightOverride(220)
				[
					SNew(SBorder)
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor::White)
					[
						SAssignNew(TableText, STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 48))
					]
				]
			]

			// Status / instructions
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(6)
			[
				SAssignNew(StatusText, STextBlock)
				.Font(InfoFont)
				.ColorAndOpacity(FLinearColor(0.9f, 0.95f, 0.9f, 1.f))
			]

			// Hand
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(20)
			[
				SAssignNew(HandBox, SHorizontalBox)
			]

			// New Game button
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(20)
			[
				SNew(SButton)
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				.ContentPadding(FMargin(30, 12))
				.OnClicked(this, &SCardGameWidget::OnNewGameClicked)
				[
					SNew(STextBlock)
					.Font(InfoFont)
					.Text(LOCTEXT("NewGame", "New Game"))
				]
			]
		]
	];

	NewGame();
}

void SCardGameWidget::NewGame()
{
	Deck.Empty();
	for (int32 i = 0; i < 52; ++i)
	{
		Deck.Add(i);
	}

	// Fisher-Yates shuffle.
	for (int32 i = Deck.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Deck.Swap(i, j);
	}

	Score = 0;
	Plays = 0;
	bGameOver = false;

	// Deal the table card and a full hand.
	TableCard = Deck.Pop(EAllowShrinking::No);

	Hand.Init(-1, HandSize);
	for (int32 s = 0; s < HandSize; ++s)
	{
		DrawIntoSlot(s);
	}

	Refresh();
}

void SCardGameWidget::DrawIntoSlot(int32 Slot)
{
	if (Deck.Num() > 0)
	{
		Hand[Slot] = Deck.Pop(EAllowShrinking::No);
	}
	else
	{
		Hand[Slot] = -1;
	}
}

FReply SCardGameWidget::OnCardClicked(int32 Slot)
{
	if (bGameOver || !Hand.IsValidIndex(Slot) || Hand[Slot] < 0)
	{
		return FReply::Handled();
	}

	const int32 Played = Hand[Slot];
	const bool bHigher = RankOf(Played) > RankOf(TableCard);
	if (bHigher)
	{
		++Score;
	}
	++Plays;

	TableCard = Played;
	DrawIntoSlot(Slot);

	// Game over when the hand is empty and the deck is exhausted.
	bool bAnyCards = false;
	for (int32 c : Hand)
	{
		if (c >= 0) { bAnyCards = true; break; }
	}
	if (!bAnyCards)
	{
		bGameOver = true;
	}

	Refresh();
	return FReply::Handled();
}

FReply SCardGameWidget::OnNewGameClicked()
{
	NewGame();
	return FReply::Handled();
}

void SCardGameWidget::Refresh()
{
	if (ScoreText.IsValid())
	{
		ScoreText->SetText(FText::FromString(
			FString::Printf(TEXT("Score: %d / %d     Deck: %d"), Score, Plays, Deck.Num())));
	}

	if (TableText.IsValid())
	{
		TableText->SetText(FText::FromString(CardLabel(TableCard)));
		TableText->SetColorAndOpacity(FSlateColor(SuitColor(TableCard)));
	}

	if (StatusText.IsValid())
	{
		StatusText->SetText(bGameOver
			? FText::FromString(FString::Printf(TEXT("Round over! Final score %d. Tap New Game."), Score))
			: FText::FromString(TEXT("Tap a card that beats the table card.")));
	}

	// Rebuild the hand row.
	if (HandBox.IsValid())
	{
		HandBox->ClearChildren();
		const FSlateFontInfo CardFont = FCoreStyle::GetDefaultFontStyle("Bold", 34);

		for (int32 s = 0; s < Hand.Num(); ++s)
		{
			const int32 Card = Hand[s];
			HandBox->AddSlot().AutoWidth().Padding(8)
			[
				SNew(SBox).WidthOverride(110).HeightOverride(150)
				[
					SNew(SButton)
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.IsEnabled(Card >= 0 && !bGameOver)
					.OnClicked(this, &SCardGameWidget::OnCardClicked, s)
					[
						SNew(STextBlock)
						.Font(CardFont)
						.ColorAndOpacity(FSlateColor(Card >= 0 ? SuitColor(Card) : FLinearColor::Gray))
						.Text(FText::FromString(Card >= 0 ? CardLabel(Card) : TEXT("--")))
					]
				]
			];
		}
	}
}

#undef LOCTEXT_NAMESPACE
