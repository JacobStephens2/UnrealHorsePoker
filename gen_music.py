import wave, struct, math

SR = 44100
chord_dur = 2.0
prog = [
    [261.63, 329.63, 392.00, 493.88],  # Cmaj7
    [220.00, 261.63, 329.63, 392.00],  # Am7
    [293.66, 349.23, 440.00, 523.25],  # Dm7
    [196.00, 246.94, 293.66, 349.23],  # G7
]
bass = [130.81, 110.00, 146.83, 98.00]  # C3, A2, D3, G2

def env(t, dur, atk=0.04, rel=0.30):
    if t < atk:
        return t / atk
    if t > dur - rel:
        return max(0.0, (dur - t) / rel)
    return 1.0

samples = []
for ci, notes in enumerate(prog):
    n = int(chord_dur * SR)
    for i in range(n):
        t = i / SR
        e = env(t, chord_dur)
        s = 0.0
        # mellow pad (chord)
        for f in notes:
            s += 0.06 * math.sin(2 * math.pi * f * t)
        # bass
        s += 0.13 * math.sin(2 * math.pi * bass[ci] * t) * e
        # gentle arpeggio: one chord note every 0.5s with a quick pluck decay
        ai = int(t / 0.5) % len(notes)
        at = t - int(t / 0.5) * 0.5
        pluck = math.exp(-at * 6.0)
        s += 0.07 * math.sin(2 * math.pi * notes[ai] * 2 * t) * pluck
        s *= e
        samples.append(s)

peak = max(abs(x) for x in samples) or 1.0
scale = 0.82 / peak
data = b''.join(struct.pack('<h', int(max(-1, min(1, x * scale)) * 32767)) for x in samples)

out = '/Users/admin/UnrealProjects/CardGame/SourceAudio/poker_theme.wav'
import os
os.makedirs(os.path.dirname(out), exist_ok=True)
w = wave.open(out, 'wb')
w.setnchannels(1)
w.setsampwidth(2)
w.setframerate(SR)
w.writeframes(data)
w.close()
print('wrote', out, len(samples) / SR, 'seconds,', len(data), 'bytes')
