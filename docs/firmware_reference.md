# Firmware control reference

Choose a firmware below, or match its **Reverse LED color in the selector**.
Colors after launch belong to that firmware, not AuroraSwitch.

Knob names refer to the original Aurora panel. **Shift + control** means hold Shift
while using that control. CCW / CW mean counterclockwise / clockwise.

This covers the current 12-entry development catalog. Entries marked **development**
are not in the published seven-entry selector. Control descriptions come from
author manuals/notes or versioned source; they are not a claim that every function
has been physically tested. Only exact supported files appear in the menu.

## Color lookup

| Selector color | Firmware | What it does | Availability |
| --- | --- | --- | --- |
| Blue | [FDN 1.2.2](#fdn-122--blue) | Conventional reverb | Release |
| Green | [Aurora 1.4.4](#aurora-144--green) | Spectral reverb | Release |
| Cyan | [EchoGarden 0.3.1](#echogarden-031--cyan) | Multi-line echo and ambience | Release |
| Magenta | [CloudscapeX](#cloudscapex--magenta) | Modulated delay, diffusion and filter delay | Release |
| Amber | [The Oscillator Is a Lie 0.0.2](#the-oscillator-is-a-lie-002--amber) | Aliasing/ring-mod oscillator | Release |
| Yellow | [Flux Capacitor 0.3.0](#flux-capacitor--yellow-orange-or-violet) | Tape pitch, delay and coloration | Release |
| White | [Morse 0.2.0](#morse--white-or-mint) | Clocked rhythmic VCA | Release |
| Orange | [Flux Capacitor 0.1.0](#flux-capacitor--yellow-orange-or-violet) | Tape pitch, stop, wow/flutter | Development |
| Violet | [Flux Capacitor 0.2.0](#flux-capacitor--yellow-orange-or-violet) | Adds tape delay | Development |
| Mint | [Morse 0.1.0](#morse--white-or-mint) | Three-pattern rhythmic VCA | Development |
| Pale red | [Tempest 1.0.0](#tempest-100--pale-red) | Multi-bank distortion | Development |
| Azure | [FataMorgana RAM experiment](#fatamorgana-ram-experiment--azure) | Dual wavetable synthesizer | Development |

Reverse selects → Freeze loads/verifies → **wait for Freeze green** → Shift launches.
Power cycle to return. See the [user guide](user_guide.md) for files and installation.

## FDN 1.2.2 — Blue

Stereo feedback-delay-network reverb. The original panel labels map to the
FDN overlay as follows:

### Knobs and CV

| Panel knob / matching CV | Function |
| --- | --- |
| Warp | **Pitch:** modulation amount; CCW disables modulation |
| Time | Decay; short/comb-like CCW, near-infinite CW |
| Blur | **Input Level:** 25–150% by default |
| Reflect | **Highpass:** wet-signal high-pass filtering, increasing CW |
| Mix | Dry/wet balance |
| Atmosphere | **Damp:** damping increases CW |

### Buttons and gates

| Button | Function |
| --- | --- |
| Reverse | Reverse incoming audio |
| Freeze | Hold captured audio |
| Shift + Reverse | Swap DSP order: reverb last ↔ reverse last |
| Shift + Freeze | Switch input-level range: 25–150% ↔ 0–150% |

### Notes and sources

CVs follow the remapped controls; the guide specifies ±5 V and a 0.4 V gate
threshold for Reverse/Freeze. Left input normals to both channels without a
right input cable. Running LEDs use purple/gold, unlike the blue selector color.
Start with moderate Time and raise it gradually: high settings feed back.

Source: Qu-Bit's **FDN getting started** guide, including its front-panel overlay,
available with the [official FDN documentation](https://www.qubitelectronix.com/alternate-firmware/p/fdn-verb).

## Aurora 1.4.4 — Green

Stock spectral reverb: pitch manipulation, spectral smearing and stereo delays.

### Knobs and CV

| Panel knob / matching CV | Function |
| --- | --- |
| Warp | Pitch shift ±3 octaves; noon is unshifted; CV tracks 1 V/oct |
| Time | Amplitude-domain smearing / spectral tail |
| Blur | Frequency-domain smearing |
| Reflect | Increasing multi-delay time zones, from no extra delay CCW |
| Mix | Dry ↔ wet |
| Atmosphere | Spectral/time filtering: underwater below noon, brighter then high-pass above noon |

### Buttons and shifted controls

| Button / shifted control | Function |
| --- | --- |
| Reverse | Reverse audio; active LED green; state retained |
| Freeze | Sustain the current spectrum; Warp/Time/Blur/Atmosphere/Mix still work |
| Shift + Mix | Input gain, 0.5–4×; blue indicates default 1× |
| Shift + Reflect | Stereo enhancement, 0–100%; blue indicates default 75% |
| Shift + Reverse | Cycle FFT size: 4096 blue → 2048 green → 1024 cyan → 512 purple |
| Shift + Reverse, held 2 seconds | Factory reset UI/options defaults; white confirmation animation |
| Shift + Freeze | Reload USB settings; successful reload flashes white |

### Lights and settings

Changing FFT size loses the frozen spectrum; freeze again afterward. Larger FFTs
give smoother pitch/tails with more latency. Warp octave positions show green/blue;
off-octave positions show green/purple. CV ranges are ±5 V; gates use 0.4 V threshold.

USB `options.txt` controls DSP order, Freeze forcing full-wet, dry latency
compensation, always-on blur, Warp quantization and octave deadzones. Defaults:
`DSP_ORDER=1`, `FREEZE_WET=0`, `LATENCY_COMP=0`, `ALWAYS_BLUR=1`,
`QUANTIZE_WARP=1`, `WARP_DEADZONES=1`. Quantization also accepts `2` for knob + CV.
Keep existing settings files; firmware updates happen at boot, not on reload.

Source: Qu-Bit's **Aurora manual v1.7.4**, which explicitly covers firmware 1.4.4
(supplied PDF filename `Aurora_v1.7-1.pdf`). See the
[official Aurora page](https://www.qubitelectronix.com/alternate-firmware/p/aurora-spectral-reverb)
for the full manual, options and calibration procedure. Calibration is maintenance,
not a normal performance control.

## EchoGarden 0.3.1 — Cyan

Multi-line echoes with diffusion/reverb around the repeats.

### Knobs

| Panel knob | Function |
| --- | --- |
| Warp | Echo spacing: tight → roughly 1× / 1.33× / 1.67× / 2× |
| Time | Base echo time, about 15 ms–1.8 s |
| Blur | Reverb/diffusion depth |
| Reflect | Feedback, up to about 96% normally |
| Mix | Dry/wet |
| Atmosphere | Select 1–4 active echo lines |

### Buttons, gates and lights

Freeze latches a held feedback texture with new input removed; its LED is yellow.
Reverse latches ping-pong/cross-feedback; its LED is green. **Hold Shift** for
Bloom: extra reverb send and a longer tail while held. Each gate temporarily
inverts its corresponding stored button state, rather than simply enabling it.

Moving a knob shows a six-LED value meter. Parameter indications: Time blue,
Reflect red, Mix yellow, Atmosphere green, Blur blue and Warp yellow.
Try Time 40%, Reflect 50%, Mix 60%, one line, Blur 10%, Warp low.

### Sources

Source: author-supplied **EchoGarden v0.3.1 Parameter Manual**. It does not specify
CV scaling/ranges; those remain unconfirmed here. Do not infer them from stock.

## CloudscapeX — Magenta

The supplied **Cloudscape v1.6** manual describes this file's controls; the user
identified it as the only released version. The BIN itself is named CloudscapeX.

### Knobs

| Panel knob | Normal / Filter Delay mode |
| --- | --- |
| Warp | Timing spread and modulation / resonance plus cutoff motion |
| Time | Delay about 10 ms–1.05 s / about 18–780 ms |
| Blur | Diffusion/reverb / filter cutoff about 85 Hz–18 kHz |
| Reflect | Network feedback / delay feedback |
| Mix | Dry/wet |
| Atmosphere | 1–4 lines / LP → BP → HP → notch filter selection |

### Buttons and gates

| Button | Latched effect / active LED |
| --- | --- |
| Freeze | Hold cloud / white |
| Reverse | Rotate or ping-pong routing / cyan |
| Shift + Freeze | Lush Reverb / blue |
| Shift + Reverse | Filter Delay / yellow |

All four effects latch independently. Freeze alternates white/blue when hold and
Lush are both active; Reverse alternates cyan/yellow for rotation plus Filter Delay.
Gates temporarily invert the stored Freeze/Reverse states. Parameter LEDs:
Time blue, Reflect red, Mix yellow, Atmosphere green, Blur cyan, Warp magenta.
Try Time 30%, Reflect 40%, Mix 55%, Atmosphere 50%, Blur 30%, Warp 25%.

### Sources

Source: author-supplied **Cloudscape v1.6 One Page Parameter Manual**.
CV scaling/ranges are not specified in that manual.

## The Oscillator Is a Lie 0.0.2 — Amber

An oscillator voice, not a reverb. It produces pitch by aliasing a faster
oscillator or using ring-modulation sidebands.

### Knobs

| Panel knob | Function |
| --- | --- |
| Warp | Pitch offset; latest supplied note says ±2 octaves; CV is V/oct |
| Time | Carrier-to-target ratio, 1–8× |
| Blur | Output level without clipping; clipping drive when enabled |
| Reflect | Low-pass resonance |
| Mix | CW: main output; CCW: blend raw modulator left / raw carrier right |
| Atmosphere | Low-pass cutoff; minimum tracks target pitch, maximum about 10 kHz |

### Buttons and gates

| Button | Function / LED |
| --- | --- |
| Reverse | Carrier waveform: sine blue → triangle magenta → saw yellow → square cyan |
| Freeze | Aliasing green ↔ ring-modulation blue |
| Shift + Reverse | Clipping: off → before filter white → after filter red |
| Shift + Freeze | Mix routing: normal off → main-only left white → main-only right red |

The later author note adds Reverse/Freeze gate control of waveform/mode.
Top-row colors indicate pitch/ratio consonance; lower LEDs react to cutoff,
resonance and gain. Start with Blur above zero to hear it.

### Sources and version caveat

Source: supplied Discord author notes dated February 10–11, 2026. The later note
changes the original ±1-octave Warp description and adds level control. Their
exact correspondence to the supplied 0.0.2 BIN has not been independently verified;
other CV mappings and audio-input use are undocumented, not assumed unused.

## Flux Capacitor — Yellow, Orange or Violet

Stereo tape-style processing. Common controls:

### Knobs and CV

| Panel knob / CV | Function |
| --- | --- |
| Warp | Smooth pitch ±12 semitones; CV 1 V/oct |
| Time | Delay 1 ms–2 s in 0.2/0.3; unused in 0.1 |
| Blur | Flutter depth |
| Reflect | Wow rate, 0.1–2 Hz |
| Mix | Dry/wet |
| Atmosphere | Tone darkening, saturation and feedback in 0.3; unused in 0.1/0.2 |

### Buttons, gates and lights

Freeze toggles a roughly 1.5-second tape stop/start; Freeze gate forces stop while
high. A stopped wet signal becomes silent. Fully dry Mix temporarily becomes wet
during stopping/starting. Reverse, its gate and Shift are unused. Active CVs add
to their knobs and clamp. Arc LEDs indicate pitch: amber up, cyan down; Freeze
red follows braking, with white flashes at the dry-Mix boundary.

### Version differences

| Version / selector color | Difference |
| --- | --- |
| 0.1.0 / Orange | Pitch, braking, wow/flutter only |
| 0.2.0 / Violet | Adds delay with fixed feedback |
| 0.3.0 / Yellow | Adds Atmosphere coloration/feedback |

Sources: [0.3.0 guide](https://github.com/DaveParr/aurora-flux-capacitor/blob/v0.3.0/README.md)
and versioned `main.cpp` / `tape_delay.h` at tags 0.1.0 and 0.2.0.

## Morse — White or Mint

Clocked stereo VCA: **audio input + clock into Freeze gate + Mix above zero**.
Freeze button alone does not clock it.

### Knobs and buttons

| Panel control | Function |
| --- | --- |
| Time + CV | Pattern selection |
| Mix | Output level |
| Blur | Envelope attack/decay balance in 0.2; unused in 0.1 |
| Warp, Reflect, Atmosphere | Unused |
| Reverse button / gate | Reset pattern |
| Shift + Freeze | Cycle clock ratio: /8 → /4 → /2 → ×1 → ×2 → ×4 → ×8 |

### Gates and lights

Other knob CVs are unused. While Shift is held, Freeze indicates ratio:
violet /8, blue /4, light blue /2, white ×1, yellow ×2, orange ×4, red ×8.
Otherwise Freeze flashes on ticks; Reverse flashes on reset. Arc position and
brightness show pattern steps/envelope.

### Version differences

**0.1.0 (Mint):** three patterns—dotted-8th groove, slow pulse, triplet feel;
instant attack with decay. **0.2.0 (White):** twelve patterns and Blur-controlled
attack/decay. Pattern changes apply at a step boundary or reset in linked source;
the README's “next Reset” wording is incomplete.

Sources: [0.2.0 guide](https://github.com/DaveParr/Aurora-Morse/blob/v0.2.0/README.md)
and `main.cpp`, `pattern.h`, `envelope.h` at tags 0.1.0/0.2.0.

## Tempest 1.0.0 — Pale red

Stereo distortion: low frequencies bypass the distortion through a crossover;
the high band is driven, filtered and recombined. **Mix is not dry/wet.**

### Knobs and CV

| Panel knob / CV | Function |
| --- | --- |
| Warp | Drive, 1–100× |
| Time | Pre-distortion high-pass, 20 Hz–2 kHz |
| Blur | Algorithm variation |
| Reflect | Bipolar bias; center deadzone; reduced range in Standard mode |
| Mix | Crossover, 20–500 Hz |
| Atmosphere | Post-distortion low-pass, 20 Hz–20 kHz |
| Shift + Mix | Output trim, 0–2× |

### Buttons and gates

Reverse advances algorithms; Freeze goes back. Shift + Reverse advances banks;
Shift + Freeze cycles Standard (green), tube (orange), odd/inharmonic (purple).
Hold Freeze **5 seconds without Shift** to toggle cabinet simulation; three
white blinks confirm. Freeze gate holds an input sample; Reverse gate adds
octave-up rectification. CVs add to knobs (±5 V).

Reverse shows bank color; Freeze shows mode color. Arc LEDs show algorithm and
distortion level. Settings autosave after two seconds without changes.

### Algorithm banks

| Bank / running color | Algorithms, in order |
| --- | --- |
| Analog / Orange | SftClp, HardClp, StrgrC, Valve, Antipol, Disto |
| Fold / Purple | WavFld, Crest, Sstrgi, Fezz, Fuss, Cross |
| Geometric / Yellow | VariWS, SmartE, Vshpe, Square, Cubic, Anti+ |
| LoFi / Cyan | Bitcrsh, DwnSpl, Lowfi, Lowrfi, ChastB, SuspB |
| Chaos / Red | Fractl, Zippy, Bounce, NoizE, BrwnNz, Attrac |
| Temporal / Green | Filter, TimeFr, Vinyl, Clicks, Jelly, Accum |

Sources: [version-pinned controls](https://github.com/jfriess/Aurora-Firmwares/blob/d5504d76370c370fdb40adcf755d8a4b9c07ee6b/Tempest/README.md)
and [algorithm details](https://github.com/jfriess/Aurora-Firmwares/blob/d5504d76370c370fdb40adcf755d8a4b9c07ee6b/Tempest/docs/algorithms.md).

## FataMorgana RAM experiment — Azure

Dual wavetable synth: oscillator A left, B right; external audio can modulate or
be waveshaped. Only the exact development RAM build is admitted, not upstream QSPI.

### Knobs and CV

| Panel control | Normal / hold Shift |
| --- | --- |
| Warp + CV | Pitch, V/oct (0 V = C0) / B offset ±24 semitones |
| Time + CV | X wave position / sub A level |
| Reflect + CV | Y wave position / sub B level |
| Atmosphere + CV | Z wave position / no separate shifted function documented |
| Blur + CV | Effect depth / choose FM, ring mod, bitcrush, wavefold, overdrive |
| Mix knob | Unison detune 0–5% / output trim |
| Mix CV | VCA: 0 V unity, negative attenuates, positive boosts |

### Buttons and gates

Freeze toggles smooth/glitchy morphing (green/red). Reverse tap selects A/B/both
(orange/cyan/purple); hold **5 seconds** to copy A's wave positions to B.
Shift + Freeze toggles external waveshaping; Shift + Reverse toggles clean/degraded
tone. Reverse gate randomizes both XYZ positions; Freeze gate is reserved.

### Lights and pickup

After changing oscillator selection, knobs use pickup: move to the stored position
before editing. Warp also uses pickup after Shift. Effect colors: FM green,
ring mod yellow, bitcrush red, wavefold magenta, overdrive orange.
Arc pairs show X red, Y green, Z blue; shifted LEDs show subs/effect/trim.

### USB tables

USB tables: root `1.wav`–`8.wav`, each mono 16-bit PCM, 64 consecutive 256-sample
waves (16,384 samples). Without tables, a generated cube is used. Loading status:
yellow busy, green success, red missing, orange invalid WAV, magenta compressed,
blue stereo, white wrong depth, cyan too short. **USB table loading remains
unverified for this RAM build.** Tempest/Fata share a settings sector with different
formats; switching can change retained settings.

Source: [pinned FataMorgana guide](https://github.com/jfriess/Aurora-Firmwares/blob/d5504d76370c370fdb40adcf755d8a4b9c07ee6b/FataMorgana/README.md)
and `src/FataMorgana.cpp` at the same commit.

## Outside the selector catalog

These have **no selector color** and cannot be launched by the current selector.

- **Dirt Verb 1.1:** Warp decay/feedback, Time overdrive, Blur damping, Reflect
  chorus rate, Mix dry/wet, Atmosphere pitch-shifter mix. Reverse toggles chorus;
  Freeze toggles pitch shifting. Author notes describe gold at infinite feedback,
  white when overdrive bypasses and red as drive increases. Shift/CV/gate behavior
  is not documented in the supplied notes. Its QSPI execution requires separate
  support; do not rename it to a supported file.
- **Aurora HP-filter variant:** stock behavior plus Shift + Atmosphere dry-input
  high-pass and Shift + Blur wet level. Its supplied BIN exceeds staging capacity.
- **HSO, tape-delay WIP and Phazr:** mentioned in community notes, but not onboarded;
  controls and compatibility are not covered here.

## Reference maintenance

Menu colors come from [`firmware/images.hpp`](../firmware/images.hpp).
Discord-only summaries use the author notes/manuals supplied during onboarding;
they are paraphrases, not bundled manuals or proof of binary behavior. Downloads
are listed [separately](alternative_firmware.md). Update this reference when a
catalog entry or version-specific control changes; distinguish unknown from unused.
