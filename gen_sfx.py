import wave, struct, math, random, os

SR = 44100
OUT = "/Users/admin/UnrealProjects/CardGame/SourceAudio"
os.makedirs(OUT, exist_ok=True)
random.seed(7)

def write(name, samples):
    peak = max((abs(x) for x in samples), default=1.0) or 1.0
    scale = 0.82 / peak
    data = b''.join(struct.pack('<h', int(max(-1, min(1, x * scale)) * 32767)) for x in samples)
    w = wave.open(os.path.join(OUT, name), 'wb')
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
    w.writeframes(data); w.close()
    print("wrote", name, round(len(samples)/SR, 3), "s")

def tone(freq, t, kind='sine'):
    if kind == 'sine':
        return math.sin(2*math.pi*freq*t)
    if kind == 'tri':
        ph = (freq*t) % 1.0
        return 4*abs(ph-0.5)-1
    return math.sin(2*math.pi*freq*t)

# --- deal: short filtered noise flick (~90ms) ---
s = []
n = int(0.09*SR)
prev = 0.0
for i in range(n):
    t = i/SR
    e = math.exp(-t*40)
    white = random.uniform(-1, 1)
    prev = 0.6*prev + 0.4*white  # low-pass for a softer 'flick'
    s.append(prev * e)
write("sfx_deal.wav", s)

# --- chip: two quick metallic clicks (~150ms) ---
s = []
n = int(0.15*SR)
for i in range(n):
    t = i/SR
    e = 0.0
    for onset in (0.0, 0.06):
        if t >= onset:
            dt = t-onset
            e += math.exp(-dt*55)
    v = (0.6*tone(2300, t) + 0.4*tone(3400, t)) * e
    s.append(v)
write("sfx_chip.wav", s)

# --- check: soft low knock (~70ms) ---
s = []
n = int(0.07*SR)
for i in range(n):
    t = i/SR
    e = math.exp(-t*45)
    s.append((0.8*tone(190, t) + 0.2*tone(380, t)) * e)
write("sfx_check.wav", s)

# --- win: rising arpeggio C5-E5-G5-C6 (~0.6s) ---
s = []
notes = [523.25, 659.25, 783.99, 1046.50]
step = 0.13
total = step*len(notes) + 0.25
n = int(total*SR)
for i in range(n):
    t = i/SR
    idx = int(t/step)
    v = 0.0
    for k, f in enumerate(notes):
        onset = k*step
        if t >= onset:
            dt = t-onset
            v += 0.5*(tone(f, t, 'tri')) * math.exp(-dt*4.5)
    s.append(v)
write("sfx_win.wav", s)

# --- lose: descending two-tone (~0.45s) ---
s = []
seq = [(392.00, 0.0), (293.66, 0.18)]
total = 0.45
n = int(total*SR)
for i in range(n):
    t = i/SR
    v = 0.0
    for f, onset in seq:
        if t >= onset:
            dt = t-onset
            v += 0.6*tone(f, t) * math.exp(-dt*5.0)
    s.append(v)
write("sfx_lose.wav", s)

print("done")
