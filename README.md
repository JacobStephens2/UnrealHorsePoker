# HORSE Poker (Unreal Engine, Android)

A sample **HORSE poker** game built with **Unreal Engine 5.7**, packaged for **Android**. The UI is written entirely in C++ with Slate — no Blueprint assets — so the whole game is plain text in `Source/`.

## The game

Heads-up HORSE poker against a simple AI, with chips and fixed-limit betting. Each hand rotates through the five variants:

| | Variant | Deal | Goal |
|---|---------|------|------|
| **H** | Texas Hold'em | 2 hole + 5 community | best high hand |
| **O** | Omaha (Hi) | 4 hole + 5 community, use **exactly** 2 + 3 | best high hand |
| **R** | Razz | 7-card stud | best **A-5 low** (straights/flushes ignored, Ace low) |
| **S** | Seven-card Stud | 7 cards | best high hand |
| **E** | Stud Hi-Lo (8 or better) | 7 cards | pot **split** between best high and best qualifying low (8-or-better) |

- Antes each hand, then fixed-limit betting on every street (pre-flop/flop/turn/river for the board games; 3rd→7th street for the stud games). Small bet early, big bet on later streets, capped raises.
- Tap **Fold / Check / Call / Bet / Raise**. The opponent acts by a hand-strength heuristic per variant (high strength, low strength, or the better of the two for Hi-Lo).
- Opponent's hole cards stay hidden until showdown; in the stud games their up-cards are visible as they're dealt.
- Hi-Lo hands split the pot; Razz awards the low; the rest award the high. Win all the opponent's chips (or lose yours) to end the game.

## Hand evaluation

`Source/CardGame/PokerEval.h` is a dependency-free C++ evaluator (so it can be unit-tested outside the engine):

- Best 5-of-N **high** hand (royal/straight flush down to high card, with correct kicker tie-breaks and the A-2-3-4-5 wheel).
- **Omaha** high using exactly two hole cards + three board cards.
- **A-5 lowball** for Razz and the low half of Stud Hi-Lo, with the **eight-or-better** qualifier.

## Layout

| Path | Purpose |
|------|---------|
| `Source/CardGame/PokerEval.h` | Pure-C++ poker hand evaluation (high / Omaha / A-5 low / 8-or-better) |
| `Source/CardGame/SHorseGameWidget.*` | The Slate UI, HORSE rotation, betting state machine, AI, showdown/pot split |
| `Source/CardGame/CardGamePlayerController.*` | Adds the widget to the viewport, sets touch/UI input |
| `Source/CardGame/CardGameGameModeBase.*` | Default game mode wiring |
| `Config/` | Project + Android packaging settings |
| `Content/Maps/Main.umap` | Minimal startup level |

## Building the Android APK

Requires UE 5.7 with the Android platform component, plus an Android SDK/NDK (NDK r27 `27.2.12479018`, build-tools 35.0.1, platform-34) and JDK 17. Export `ANDROID_HOME`, `ANDROID_NDK_ROOT`, `NDKROOT`, `NDK_ROOT`, `JAVA_HOME`, then:

```bash
UE="/path/to/UE_5.7"
PROJ="$PWD/CardGame.uproject"

# Compile the editor module (first time, so the editor sees the C++ classes)
"$UE/Engine/Build/BatchFiles/Mac/Build.sh" CardGameEditor Mac Development -project="$PROJ" -waitmutex

# Build + cook + package a self-contained APK (data inside the APK, no OBB)
"$UE/Engine/Build/BatchFiles/RunUAT.command" BuildCookRun \
  -project="$PROJ" -platform=Android -cookflavor=ASTC -clientconfig=Development \
  -build -cook -stage -package -pak \
  -archive -archivedirectory="$PWD/Archive" -nodebuginfo -nocompileeditor
```

Output: `Archive/Android_ASTC/CardGame-arm64.apk`. Install with:

```bash
adb install -r Archive/Android_ASTC/CardGame-arm64.apk
```

The evaluator can be sanity-checked on its own:

```bash
clang++ -std=c++17 test_poker.cpp -o test_poker && ./test_poker
```

> `bPackageDataInsideApk=True` (in `Config/DefaultEngine.ini`) bundles game data into the APK so a sideloaded build runs without an OBB file or Google Play Store key.
