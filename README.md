# Card Game (Unreal Engine, Android)

A sample 2D card game built with **Unreal Engine 5.7**, packaged for **Android**. The UI is written entirely in C++ with Slate — no Blueprint assets — so the whole game is plain text in `Source/`.

## The game — "Higher or Lower"

- A shuffled 52-card deck, a face-up **table card**, and a **hand of 5** cards shown as tappable buttons (red/black suits, ♠ ♥ ♦ ♣).
- Tap a hand card that **ranks higher** than the table card to score a point. The played card becomes the new table card and a replacement is drawn.
- Score and remaining-deck count show at the top. When the deck and hand are exhausted the round ends — tap **New Game** to reshuffle.
- Portrait orientation, green felt background.

## Layout

| Path | Purpose |
|------|---------|
| `Source/CardGame/SCardGameWidget.*` | The Slate card-game UI and all game logic |
| `Source/CardGame/CardGamePlayerController.*` | Adds the widget to the viewport, sets touch/UI input |
| `Source/CardGame/CardGameGameModeBase.*` | Default game mode wiring |
| `Source/CardGame.Target.cs`, `CardGameEditor.Target.cs` | Game / Editor build targets (`BuildSettingsVersion.V6`) |
| `Config/` | Project + Android packaging settings |
| `Content/Maps/Main.umap` | Minimal startup level |
| `make_map.py` | Headless map creation (Python commandlet) |

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

> `bPackageDataInsideApk=True` (in `Config/DefaultEngine.ini`) bundles game data into the APK so a sideloaded build runs without an OBB file or Google Play Store key.
