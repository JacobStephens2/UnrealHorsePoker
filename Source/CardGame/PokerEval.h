#pragma once

// Pure-C++ poker hand evaluation (no Unreal dependencies so it can be unit-tested
// standalone). Cards are ints 0..51: rank = card % 13 (0=Two .. 12=Ace), suit = card / 13.
//
// Provides:
//   - High-hand evaluation (best 5 of N) for Hold'em / Stud.
//   - Omaha high (exactly 2 hole + 3 board).
//   - A-5 lowball (Ace low, straights/flushes ignored) for Razz and the low half of Stud Hi-Lo,
//     with an "eight-or-better" qualifier.

#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>

namespace poker
{
	inline int CardRank(int c) { return c % 13; } // 0=2 .. 8=10, 9=J,10=Q,11=K,12=A
	inline int CardSuit(int c) { return c / 13; } // 0..3

	enum EHighCat
	{
		HC_HighCard = 0, HC_Pair, HC_TwoPair, HC_Trips, HC_Straight,
		HC_Flush, HC_FullHouse, HC_Quads, HC_StraightFlush
	};

	struct EvalResult
	{
		uint64_t score = 0; // bigger == stronger as a HIGH hand
		int cat = 0;        // EHighCat for high mode; "no-pair/pair/..." for low mode
		int topCard = 0;    // highest value among the 5 chosen cards (value space of the mode)
		bool noPair = false;
		std::array<int, 5> used{ { -1, -1, -1, -1, -1 } }; // the 5 cards forming the hand
	};

	// Evaluate exactly 5 cards. lowMode: ignore straights/flushes and treat Ace as low.
	inline EvalResult eval5(const int* cs, bool lowMode)
	{
		int cnt[13] = { 0 };
		int suit[4] = { 0 };
		for (int i = 0; i < 5; ++i)
		{
			const int r = CardRank(cs[i]);
			const int v = lowMode ? ((r == 12) ? 0 : r + 1) : r; // low: A=0,2=1,..,K=12
			cnt[v]++;
			suit[CardSuit(cs[i])]++;
		}

		bool flush = false;
		for (int s = 0; s < 4; ++s) if (suit[s] == 5) flush = true;

		int present = 0;
		for (int v = 0; v < 13; ++v) if (cnt[v]) present |= (1 << v);

		bool straight = false;
		int shigh = -1;
		if (!lowMode)
		{
			for (int top = 12; top >= 4; --top)
			{
				bool all = true;
				for (int k = 0; k < 5; ++k) if (!(present & (1 << (top - k)))) { all = false; break; }
				if (all) { straight = true; shigh = top; break; }
			}
			// Wheel A-2-3-4-5 (A=12, 5=3) ranks as a 5-high straight.
			if (!straight && (present & (1 << 12)) && (present & 1) && (present & 2) && (present & 4) && (present & 8))
			{
				straight = true; shigh = 3;
			}
		}

		// Groups (count, value), sorted by count desc then value desc.
		std::vector<std::pair<int, int>> grp;
		for (int v = 12; v >= 0; --v) if (cnt[v]) grp.push_back({ cnt[v], v });
		std::sort(grp.begin(), grp.end(), [](const std::pair<int,int>& a, const std::pair<int,int>& b)
		{
			if (a.first != b.first) return a.first > b.first;
			return a.second > b.second;
		});

		int cat;
		if (!lowMode && straight && flush) cat = HC_StraightFlush;
		else if (grp[0].first == 4) cat = HC_Quads;
		else if (grp[0].first == 3 && grp.size() > 1 && grp[1].first >= 2) cat = HC_FullHouse;
		else if (!lowMode && flush) cat = HC_Flush;
		else if (!lowMode && straight) cat = HC_Straight;
		else if (grp[0].first == 3) cat = HC_Trips;
		else if (grp[0].first == 2 && grp.size() > 1 && grp[1].first == 2) cat = HC_TwoPair;
		else if (grp[0].first == 2) cat = HC_Pair;
		else cat = HC_HighCard;

		std::vector<int> tb;
		if (cat == HC_StraightFlush || cat == HC_Straight) tb.push_back(shigh);
		else for (const auto& g : grp) tb.push_back(g.second);
		while (tb.size() < 5) tb.push_back(-1);

		uint64_t score = (uint64_t)cat;
		for (int i = 0; i < 5; ++i) score = (score << 4) | ((uint64_t)(tb[i] + 1) & 0xF);

		EvalResult R;
		R.score = score;
		R.cat = cat;
		R.noPair = (cat == HC_HighCard);
		int hi = -1;
		for (int v = 12; v >= 0; --v) if (cnt[v]) { hi = v; break; }
		R.topCard = hi;
		for (int i = 0; i < 5; ++i) R.used[i] = cs[i];
		return R;
	}

	// Best high hand from N cards (5..7).
	inline EvalResult BestHigh(const std::vector<int>& cards)
	{
		EvalResult best;
		best.score = 0;
		const int n = (int)cards.size();
		for (int mask = 0; mask < (1 << n); ++mask)
		{
			if (__builtin_popcount(mask) != 5) continue;
			int t[5], k = 0;
			for (int i = 0; i < n; ++i) if (mask & (1 << i)) t[k++] = cards[i];
			EvalResult r = eval5(t, false);
			if (r.score > best.score) best = r;
		}
		return best;
	}

	// Best A-5 low from N cards. Smaller score == better low.
	// qualifies8 set true when the best low is "eight or better" (no pair, top card <= 8).
	inline EvalResult BestLow(const std::vector<int>& cards, bool& qualifies8)
	{
		EvalResult best;
		best.score = UINT64_MAX;
		const int n = (int)cards.size();
		for (int mask = 0; mask < (1 << n); ++mask)
		{
			if (__builtin_popcount(mask) != 5) continue;
			int t[5], k = 0;
			for (int i = 0; i < n; ++i) if (mask & (1 << i)) t[k++] = cards[i];
			EvalResult r = eval5(t, true);
			if (r.score < best.score) best = r;
		}
		// In low value space: A=0,2=1,..,8=7. Eight-or-better == no pair and top card <= 7.
		qualifies8 = best.noPair && best.topCard <= 7;
		return best;
	}

	// Omaha high: exactly 2 of the hole cards + 3 of the board.
	inline EvalResult OmahaHigh(const std::vector<int>& holes, const std::vector<int>& board)
	{
		EvalResult best;
		best.score = 0;
		const int hn = (int)holes.size();
		const int bn = (int)board.size();
		for (int hmask = 0; hmask < (1 << hn); ++hmask)
		{
			if (__builtin_popcount(hmask) != 2) continue;
			for (int bmask = 0; bmask < (1 << bn); ++bmask)
			{
				if (__builtin_popcount(bmask) != 3) continue;
				int t[5], k = 0;
				for (int i = 0; i < hn; ++i) if (hmask & (1 << i)) t[k++] = holes[i];
				for (int i = 0; i < bn; ++i) if (bmask & (1 << i)) t[k++] = board[i];
				EvalResult r = eval5(t, false);
				if (r.score > best.score) best = r;
			}
		}
		return best;
	}

	inline const char* HighCatName(int cat)
	{
		static const char* n[] = {
			"High Card", "Pair", "Two Pair", "Three of a Kind", "Straight",
			"Flush", "Full House", "Four of a Kind", "Straight Flush"
		};
		if (cat < 0 || cat > 8) return "?";
		return n[cat];
	}
}
