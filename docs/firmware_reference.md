# Firmware control reference

## Color lookup

| Selector color | Firmware | What it does | Availability |
| --- | --- | --- | --- |
| Blue | [FDN 1.2.2](#fdn-122--blue) | Conventional reverb | Release |
| Green | [Aurora 1.4.4](#aurora-144--green) | Spectral reverb | Release |
| Cyan | [EchoGarden 0.3.1](#echogarden-031--cyan) | Multi-line echo and ambience | Release |
| Magenta | [CloudscapeX](#cloudscapex--magenta) | Modulated delay, diffusion and filter delay | Release |
| Amber | [The Oscillator Is a Lie 0.0.2](#the-oscillator-is-a-lie-002--amber) | Aliasing/ring-mod oscillator | Release |
| Yellow | [Flux Capacitor 0.3.0](#flux-capacitor--yellow) | Tape pitch, delay and coloration | Release |
| White | [Morse 0.2.0](#morse--white) | Clocked rhythmic VCA | Release |
| Yellow | [Flux Capacitor 0.1.0](#flux-capacitor--yellow) | Tape pitch, stop, wow/flutter | Development |
| Yellow | [Flux Capacitor 0.2.0](#flux-capacitor--yellow) | Adds tape delay | Development |
| White | [Morse 0.1.0](#morse--white) | Three-pattern rhythmic VCA | Development |
| Pale red | [Tempest 1.0.0](#tempest-100--pale-red) | Multi-bank distortion | Development |
| Azure | [FataMorgana RAM experiment](#fatamorgana-ram-experiment--azure) | Dual wavetable synthesizer | Development |
| Red-pink | [Dirt Verb 1.1](#dirt-verb-11--red-pink) | Driven reverb with chorus and pitch shifting | Development |
| Lime | [Aurora HP-filter variant](#aurora-hp-filter-variant--lime) | Spectral reverb with dry-input high-pass | Development |

Reverse selects → Freeze verifies → **wait for Freeze green** → Shift launches.
Power cycle to return. [Setup guide](user_guide.md).

Dirt Verb and HP-filter require the opt-in `codex/qspi-dirt-verb` build, not
the published release. Virtual screening is not physical compatibility or
recovery testing. FataMorgana uses our RAM build; its duplicate QSPI option is retired.

## FDN 1.2.2 — Blue

Stereo feedback-delay-network reverb. Start with moderate Time; high settings feed back.

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

### Details and sources

- CV: ±5 V; Reverse/Freeze gate threshold: 0.4 V.
- Left input feeds both channels without a right input cable.
- Running LEDs: purple/gold.

Source: Qu-Bit's [FDN getting started guide](https://www.qubitelectronix.com/alternate-firmware/p/fdn-verb).

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

Changing FFT size clears the frozen spectrum. Larger FFTs give smoother tails but more latency.

### Details and sources

- Warp LEDs: green/blue on octave; green/purple off octave.
- CV: ±5 V; gate threshold: 0.4 V.
- Keep settings files. Reloading settings does not update firmware.

USB `options.txt` defaults:

| Setting | Default | Controls |
| --- | --- | --- |
| `DSP_ORDER` | 1 | DSP order |
| `FREEZE_WET` | 0 | Freeze forces full-wet |
| `LATENCY_COMP` | 0 | Dry latency compensation |
| `ALWAYS_BLUR` | 1 | Always-on blur |
| `QUANTIZE_WARP` | 1 | Warp quantization; 2 = knob + CV |
| `WARP_DEADZONES` | 1 | Octave deadzones |

Source: [Aurora manual v1.7.4](https://www.qubitelectronix.com/alternate-firmware/p/aurora-spectral-reverb),
covering firmware 1.4.4. Calibration is maintenance, not a performance control.

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

### Buttons and gates

| Control | Function / active LED |
| --- | --- |
| Freeze | Latch held feedback; removes new input / yellow |
| Reverse | Latch ping-pong/cross-feedback / green |
| Shift, held | Bloom: more reverb and a longer tail |
| Reverse / Freeze gates | Temporarily invert stored button states |

### Details and sources

- Knob meter: Time/Blur blue, Reflect red, Mix/Warp yellow, Atmosphere green.
- Starting point: Time 40%, Reflect 50%, Mix 60%, one line, Blur 10%, Warp low.
- CV scaling/ranges: not specified.

Source: author-supplied EchoGarden v0.3.1 Parameter Manual.

## CloudscapeX — Magenta

Modulated delay, diffusion and filter delay.

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
| Reverse / Freeze gates | Temporarily invert stored button states |

All four effects latch independently.

### Details and sources

- Combined LEDs alternate: Freeze white/blue (Hold + Lush); Reverse cyan/yellow (Rotate + Filter Delay).
- Parameter LEDs: Time blue, Reflect red, Mix yellow, Atmosphere green, Blur cyan, Warp magenta.
- Starting point: Time 30%, Reflect 40%, Mix 55%, Atmosphere 50%, Blur 30%, Warp 25%.
- CV scaling/ranges: not specified.

Source: author-supplied Cloudscape v1.6 One Page Parameter Manual;
identified as the only release paired with `CloudscapeX.bin`.

## The Oscillator Is a Lie 0.0.2 — Amber

Aliasing/ring-mod oscillator, not a reverb. Raise Blur above zero to hear it.

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
| Reverse / Freeze gates | Waveform / mode, per later author notes |

### Details and sources

- Top LEDs: pitch/ratio consonance. Lower LEDs: cutoff, resonance and gain.
- Other CV mappings and audio-input use: undocumented.
- Later notes change Warp from ±1 to ±2 octaves and add level control;
  their pairing with the 0.0.2 BIN has not been independently verified.

Source: supplied author notes, February 10–11, 2026.

## Flux Capacitor — Yellow

Stereo tape-style pitch, braking, delay and coloration.

### Knobs and CV

| Panel knob / CV | Function |
| --- | --- |
| Warp | Smooth pitch ±12 semitones; CV 1 V/oct |
| Time | Delay 1 ms–2 s in 0.2/0.3; unused in 0.1 |
| Blur | Flutter depth |
| Reflect | Wow rate, 0.1–2 Hz |
| Mix | Dry/wet |
| Atmosphere | Tone darkening, saturation and feedback in 0.3; unused in 0.1/0.2 |

### Buttons and gates

| Control | Function |
| --- | --- |
| Freeze | Tape stop/start over roughly 1.5 seconds |
| Freeze gate | Force stop while high |
| Reverse / Reverse gate / Shift | Unused |

A stopped wet signal is silent. Fully dry Mix temporarily becomes wet during braking/startup.

### Version differences

| Version | Difference |
| --- | --- |
| 0.1.0 | Pitch, braking, wow/flutter only |
| 0.2.0 | Adds delay with fixed feedback |
| 0.3.0 | Adds Atmosphere coloration/feedback |

### Details and sources

- Active CVs add to knobs and clamp.
- Arc LEDs: amber = pitch up, cyan = down. Freeze: red braking, white dry-Mix boundary flashes.

Sources: [0.3.0 guide](https://github.com/DaveParr/aurora-flux-capacitor/blob/v0.3.0/README.md)
and versioned `main.cpp` / `tape_delay.h` at tags 0.1.0 and 0.2.0.

## Morse — White

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
| Freeze gate | Clock input |
| Shift + Freeze | Cycle clock ratio: /8 → /4 → /2 → ×1 → ×2 → ×4 → ×8 |

### Version differences

| Version | Difference |
| --- | --- |
| 0.1.0 | Three patterns: dotted-8th, slow pulse, triplet; instant attack + decay |
| 0.2.0 | Twelve patterns; Blur controls attack/decay |

Pattern changes apply at a step boundary or reset, not only the next Reset.

### Details and sources

- Other knob CVs: unused.
- With Shift held, Freeze shows ratio: violet /8, blue /4, light blue /2,
  white ×1, yellow ×2, orange ×4, red ×8.
- Freeze flashes on ticks; Reverse on reset. Arc LEDs show pattern step/envelope.

Sources: [0.2.0 guide](https://github.com/DaveParr/Aurora-Morse/blob/v0.2.0/README.md)
and `main.cpp`, `pattern.h`, `envelope.h` at tags 0.1.0/0.2.0.

## Tempest 1.0.0 — Pale red

Stereo distortion with a clean low-frequency bypass. **Mix is not dry/wet.**

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

| Control | Function |
| --- | --- |
| Reverse | Next algorithm |
| Freeze | Previous algorithm |
| Shift + Reverse | Next bank |
| Shift + Freeze | Standard green → tube orange → odd/inharmonic purple |
| Freeze, hold 5 seconds without Shift | Toggle cabinet simulation; three white blinks |
| Freeze gate | Hold an input sample |
| Reverse gate | Octave-up rectification |

### Details and sources

- CVs add to knobs (±5 V).
- Reverse LED: bank; Freeze: mode; arc: algorithm/distortion level.
- Settings autosave after two seconds without changes.

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

Dual wavetable synth: A left, B right. External audio can modulate or be waveshaped.
Only the exact development RAM build is supported, not upstream QSPI.

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

| Control | Function / LED |
| --- | --- |
| Freeze | Smooth green ↔ glitchy red morphing |
| Reverse | Select A orange → B cyan → both purple |
| Reverse, hold 5 seconds | Copy A's wave positions to B |
| Shift + Freeze | Toggle external waveshaping |
| Shift + Reverse | Toggle clean/degraded tone |
| Reverse gate | Randomize both XYZ positions |
| Freeze gate | Reserved |

Knobs use pickup after oscillator selection: move to the stored position. Warp also after Shift.
Tempest/Fata share a settings sector: switching can change retained settings.

### Details and sources

- Effect LEDs: FM green, ring mod yellow, bitcrush red, wavefold magenta, overdrive orange.
- Arc pairs: X red, Y green, Z blue; shifted LEDs: subs/effect/trim.
- USB tables: root `1.wav`–`8.wav`, mono 16-bit PCM, 64 × 256 samples (16,384 total).
- Without tables, a generated cube is used. USB table loading remains unverified for this RAM build.

| Table-loading LED | Meaning |
| --- | --- |
| Yellow | Busy |
| Green | Success |
| Red | Missing |
| Orange | Invalid WAV |
| Magenta | Compressed |
| Blue | Stereo |
| White | Wrong depth |
| Cyan | Too short |

Source: [pinned FataMorgana guide](https://github.com/jfriess/Aurora-Firmwares/blob/d5504d76370c370fdb40adcf755d8a4b9c07ee6b/FataMorgana/README.md)
and `src/FataMorgana.cpp` at the same commit.

## Outside the selector catalog

No selector color; these cannot launch with the published RAM selector.

- **HSO, tape-delay WIP and Phazr:** mentioned in community notes, but not onboarded;
  controls and compatibility are not covered here.

## Dirt Verb 1.1 — Red-pink

Driven reverb with optional chorus and pitch shifting. Only the exact
`DirtVerb 1.1.bin` is admitted by the opt-in QSPI selector.

### Knobs

| Panel knob | Function |
| --- | --- |
| Warp | Decay / feedback |
| Time | Overdrive |
| Blur | Damping |
| Reflect | Chorus rate |
| Mix | Dry/wet balance |
| Atmosphere | Pitch-shifter mix |

### Buttons

| Button | Function |
| --- | --- |
| Reverse | Toggle chorus |
| Freeze | Toggle pitch shifting |

### Details and sources

- Running LEDs: gold at infinite feedback; white when overdrive is bypassed;
  red increases with drive. These are not selector menu colors.
- Shift, CV and gate behavior is not documented in the supplied author notes.
- Launch programs application QSPI. Menu re-entry requires ready USB media
  containing AuroraSwitch as the only root BIN; reset alone is not a recovery guarantee.
- Controls come from community-supplied author notes, not a reviewed source build.
  Physical handoff and stock USB recovery remain unverified.

## Aurora HP-filter variant — Lime

Stock Aurora controls with two shifted changes. Place the exact supplied
`Aurora_v1-4-6_hpfilt.bin` in `aurora/`; only the opt-in development selector
has enough reviewed staging capacity for this image.

### Shifted controls

| Control | Function |
| --- | --- |
| Shift + Atmosphere | Dry-input high-pass |
| Shift + Blur | Wet level |

Other controls follow the [stock Aurora reference](#aurora-144--green).

### Details and sources

- RAM launch; no selector QSPI programming for this payload.
- Changes are described in the supplied community notes. Physical behavior
  remains unverified; this is not an official Aurora release.
