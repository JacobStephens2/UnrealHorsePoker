"""Background music for the HORSE poker game.

A smoky swing-blues loop (distinct from the old mellow jazz-pad theme): a
walking quarter-note bass, off-beat comp "chick" chords on 2 & 4, and a bluesy
lead line over an 8-bar minor-swing form (Am - Dm - E7), all with a swing feel.
Rendered as a mono 16-bit WAV.
"""
import wave, struct, math, os

SR = 44100
BPM = 108.0
BEAT = 60.0 / BPM
SWING = BEAT / 6.0          # push off-beat eighths toward a triplet feel
BARS = 8
TOTAL = int(BARS * 4 * BEAT * SR)

buf = [0.0] * TOTAL


def add(start_sec, freq, dur_sec, amp, kind):
    a = int(start_sec * SR)
    n = int(dur_sec * SR)
    for i in range(n):
        idx = a + i
        if idx < 0 or idx >= TOTAL:
            continue
        t = i / SR
        tn = i / n
        if kind == "bass":
            env = math.exp(-tn * 1.2) * (1.0 if tn < 0.85 else (1 - tn) / 0.15)
            ph = 2 * math.pi * freq * t
            s = math.sin(ph) + 0.25 * math.sin(2 * ph)          # warm
        elif kind == "comp":
            env = math.exp(-t * 7.0)                             # short stab
            ph = 2 * math.pi * freq * t
            s = math.sin(ph) + 0.3 * math.sin(2 * ph)
        else:  # lead
            atk = 0.03
            env = (t / atk) if t < atk else max(0.0, (1 - tn) ** 0.6)
            vib = 1.0 + 0.006 * math.sin(2 * math.pi * 5.5 * t)  # light vibrato
            ph = 2 * math.pi * freq * vib * t
            s = math.sin(ph) + 0.35 * math.sin(2 * ph) + 0.12 * math.sin(3 * ph)
            s /= 1.5
        buf[idx] += amp * env * s


def beat_to_sec(beat):
    # swing: eighths landing on the "and" (x.5) get nudged late
    swing = SWING if abs((beat % 1.0) - 0.5) < 1e-6 else 0.0
    return beat * BEAT + swing


# --- 8-bar form: Am Am Dm Dm E7 E7 Am E7 ---------------------------------
# Walking bass: root-3rd-5th-6th style quarter notes per bar.
bass_bars = [
    [110.00, 130.81, 164.81, 185.00],  # Am
    [110.00, 130.81, 164.81, 146.83],  # Am -> lead to Dm
    [146.83, 174.61, 220.00, 246.94],  # Dm
    [146.83, 174.61, 220.00, 174.61],  # Dm
    [164.81, 207.65, 246.94, 293.66],  # E7
    [164.81, 207.65, 246.94, 207.65],  # E7
    [110.00, 130.81, 164.81, 185.00],  # Am
    [164.81, 207.65, 246.94, 293.66],  # E7
]
# Comp triads per bar (struck on beats 2 & 4).
comp_bars = [
    [261.63, 329.63, 440.00],  # Am: C E A
    [261.63, 329.63, 440.00],
    [293.66, 349.23, 440.00],  # Dm: D F A
    [293.66, 349.23, 440.00],
    [329.63, 415.30, 493.88],  # E7: E G# B
    [329.63, 415.30, 493.88],
    [261.63, 329.63, 440.00],
    [329.63, 415.30, 493.88],
]

for bar in range(BARS):
    base = bar * 4
    for b, f in enumerate(bass_bars[bar]):
        add(beat_to_sec(base + b), f, BEAT * 0.92, 0.16, "bass")
    for off in (1, 3):  # "chick" on 2 and 4
        for f in comp_bars[bar]:
            add(beat_to_sec(base + off), f, BEAT * 0.5, 0.05, "comp")

# Bluesy lead line (start_beat, dur_beats, freq). A blues scale: A C D Eb E G.
lead = [
    (0.0, 1.0, 329.63), (1.0, 0.5, 392.00), (1.5, 1.5, 440.00), (3.0, 1.0, 329.63),
    (5.0, 1.0, 293.66), (6.0, 2.0, 261.63),
    (8.0, 1.0, 293.66), (9.0, 1.0, 349.23), (10.0, 2.0, 440.00),
    (13.0, 1.0, 349.23), (14.0, 1.0, 293.66),
    (16.0, 1.0, 329.63), (17.0, 0.5, 415.30), (17.5, 1.5, 493.88), (19.0, 1.0, 329.63),
    (20.0, 1.0, 293.66), (22.0, 2.0, 246.94),
    (24.0, 1.0, 329.63), (25.0, 0.5, 392.00), (25.5, 1.5, 440.00), (27.0, 1.0, 329.63),
    (28.0, 1.0, 329.63), (29.0, 1.0, 293.66), (30.0, 2.0, 261.63),
]
for sb, db, f in lead:
    add(beat_to_sec(sb), f, db * BEAT * 0.95, 0.15, "lead")

# Normalize and write.
peak = max((abs(x) for x in buf), default=1.0) or 1.0
scale = 0.85 / peak
data = b''.join(struct.pack('<h', int(max(-1, min(1, x * scale)) * 32767)) for x in buf)

out = '/Users/admin/UnrealProjects/CardGame/SourceAudio/poker_theme.wav'
os.makedirs(os.path.dirname(out), exist_ok=True)
w = wave.open(out, 'wb')
w.setnchannels(1)
w.setsampwidth(2)
w.setframerate(SR)
w.writeframes(data)
w.close()
print('wrote', out, round(TOTAL / SR, 2), 'seconds,', len(data), 'bytes')
