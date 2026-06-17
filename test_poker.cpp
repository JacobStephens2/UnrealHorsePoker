#include "Source/CardGame/PokerEval.h"
#include <cstdio>
#include <string>
using namespace poker;

// card builder: rank 0..12 (0=2..12=A), suit 0..3
static int C(int rank, int suit) { return suit * 13 + rank; }
// rank constants
enum { _2=0,_3,_4,_5,_6,_7,_8,_9,_T,_J,_Q,_K,_A };
enum { S=0,H,D,Cl };

int fails = 0;
void check(bool cond, const std::string& msg) {
	printf("[%s] %s\n", cond ? "PASS" : "FAIL", msg.c_str());
	if (!cond) fails++;
}

int main() {
	// --- High hand category detection ---
	{
		std::vector<int> royal = { C(_A,S),C(_K,S),C(_Q,S),C(_J,S),C(_T,S),C(_2,H),C(_3,D) };
		auto r = BestHigh(royal);
		check(r.cat == HC_StraightFlush, "royal flush -> straight flush");
	}
	{
		std::vector<int> quads = { C(_9,S),C(_9,H),C(_9,D),C(_9,Cl),C(_K,S),C(_2,H),C(_3,D) };
		auto r = BestHigh(quads);
		check(r.cat == HC_Quads, "four nines -> quads");
	}
	{
		std::vector<int> fh = { C(_8,S),C(_8,H),C(_8,D),C(_K,Cl),C(_K,S),C(_2,H),C(_3,D) };
		auto r = BestHigh(fh);
		check(r.cat == HC_FullHouse, "eights full of kings -> full house");
	}
	{
		std::vector<int> fl = { C(_2,H),C(_5,H),C(_9,H),C(_J,H),C(_K,H),C(_3,S),C(_4,D) };
		auto r = BestHigh(fl);
		check(r.cat == HC_Flush, "five hearts -> flush");
	}
	{
		std::vector<int> wheel = { C(_A,S),C(_2,H),C(_3,D),C(_4,Cl),C(_5,S),C(_K,H),C(_K,D) };
		auto r = BestHigh(wheel);
		check(r.cat == HC_Straight, "A-2-3-4-5 -> straight (wheel)");
	}
	{
		std::vector<int> bway = { C(_T,S),C(_J,H),C(_Q,D),C(_K,Cl),C(_A,S),C(_2,H),C(_2,D) };
		auto r = BestHigh(bway);
		check(r.cat == HC_Straight, "T-J-Q-K-A -> straight (broadway)");
	}

	// --- High hand comparisons ---
	{
		std::vector<int> aces = { C(_A,S),C(_A,H),C(_5,D),C(_7,Cl),C(_9,S) };
		std::vector<int> kings = { C(_K,S),C(_K,H),C(_5,D),C(_7,Cl),C(_9,S) };
		check(BestHigh(aces).score > BestHigh(kings).score, "pair of aces beats pair of kings");
	}
	{
		std::vector<int> flushA = { C(_A,H),C(_5,H),C(_9,H),C(_J,H),C(_2,H) };
		std::vector<int> flushK = { C(_K,H),C(_5,H),C(_9,H),C(_J,H),C(_2,H) };
		check(BestHigh(flushA).score > BestHigh(flushK).score, "ace-high flush beats king-high flush");
	}
	{
		std::vector<int> straightHi = { C(_T,S),C(_J,H),C(_Q,D),C(_K,Cl),C(_A,S) };
		std::vector<int> straightLo = { C(_A,S),C(_2,H),C(_3,D),C(_4,Cl),C(_5,S) };
		check(BestHigh(straightHi).score > BestHigh(straightLo).score, "broadway straight beats wheel straight");
	}

	// --- Low (A-5) for Razz ---
	{
		// best possible low is the wheel 5-4-3-2-A
		std::vector<int> wheel = { C(_A,S),C(_2,H),C(_3,D),C(_4,Cl),C(_5,S),C(_K,H),C(_K,D) };
		bool q; auto r = BestLow(wheel, q);
		check(q == true, "wheel low qualifies for 8-or-better");
		std::vector<int> seven = { C(_A,S),C(_2,H),C(_3,D),C(_4,Cl),C(_7,S),C(_K,H),C(_K,D) };
		auto r2 = BestLow(seven, q);
		check(r.score < r2.score, "5-4-3-2-A is a better low than 7-4-3-2-A");
	}
	{
		// 6-4-3-2-A beats 6-5-3-2-A (compare second-highest card)
		std::vector<int> a = { C(_6,S),C(_4,H),C(_3,D),C(_2,Cl),C(_A,S) };
		std::vector<int> b = { C(_6,S),C(_5,H),C(_3,D),C(_2,Cl),C(_A,S) };
		bool q;
		check(BestLow(a,q).score < BestLow(b,q).score, "6-4-3-2-A better low than 6-5-3-2-A");
	}

	// --- Eight-or-better qualifier ---
	{
		bool q;
		std::vector<int> eight = { C(_8,S),C(_6,H),C(_4,D),C(_2,Cl),C(_A,S),C(_K,H),C(_K,D) };
		BestLow(eight, q);
		check(q == true, "8-6-4-2-A qualifies (eight-low)");
		std::vector<int> nine = { C(_9,S),C(_6,H),C(_4,D),C(_2,Cl),C(_A,S),C(_K,H),C(_K,D) };
		BestLow(nine, q);
		check(q == false, "9-6-4-2-A does NOT qualify (nine-low)");
		std::vector<int> paired = { C(_A,S),C(_A,H),C(_2,D),C(_3,Cl),C(_4,S),C(_K,H),C(_K,D) };
		BestLow(paired, q);
		check(q == false, "A-A-2-3-4 (pair) does NOT qualify for low");
	}

	// --- Omaha: must use exactly 2 hole + 3 board ---
	{
		// Board has 4 spades; hole has 1 spade -> cannot make a flush (need 2 hole spades).
		std::vector<int> holes = { C(_A,S),C(_K,H),C(_2,D),C(_3,Cl) };
		std::vector<int> board = { C(_5,S),C(_8,S),C(_9,S),C(_J,S),C(_2,H) };
		auto r = OmahaHigh(holes, board);
		check(r.cat != HC_Flush, "Omaha: one hole spade cannot make a flush from 4-spade board");
		// Now give two hole spades -> flush is allowed.
		std::vector<int> holes2 = { C(_A,S),C(_K,S),C(_2,D),C(_3,Cl) };
		auto r2 = OmahaHigh(holes2, board);
		check(r2.cat == HC_Flush, "Omaha: two hole spades make a flush");
	}

	printf("\n%s (%d failures)\n", fails == 0 ? "ALL TESTS PASSED" : "TESTS FAILED", fails);
	return fails == 0 ? 0 : 1;
}
