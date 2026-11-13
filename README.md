# MIDI GEN — Free Progression Rewriter by MB Audio

MIDI GEN learns the chord progression on your track and writes **8 new progressions in the same key and the same rhythm**. Each one is in a style taken from today's radio pop, rap, R&B and dance records. Click one and it plays instead of your clip; drag it into Ableton when you like it. Same hardware look as FLIPSIDE and HOOKSMITH.

![MIDI GEN](screenshot.png)

## Download (free)

Click the zip for your computer to download it (or see **[all releases](https://github.com/Maxswell5412/MIDI-GEN-VST/releases/latest)**):

| Computer | File | Formats |
|---|---|---|
| Mac (Apple Silicon + Intel) | **[⬇ MIDIGEN-Mac.zip](https://github.com/Maxswell5412/MIDI-GEN-VST/releases/latest/download/MIDIGEN-Mac.zip)** | AU + VST3 |
| Windows (64-bit) | **[⬇ MIDIGEN-Windows.zip](https://github.com/Maxswell5412/MIDI-GEN-VST/releases/latest/download/MIDIGEN-Windows.zip)** | VST3 |
| Linux (64-bit) | **[⬇ MIDIGEN-Linux.zip](https://github.com/Maxswell5412/MIDI-GEN-VST/releases/latest/download/MIDIGEN-Linux.zip)** | VST3 + standalone app |

Each zip has a short **Install** text inside. Works in Ableton Live, FL Studio, Logic Pro (AU), Bitwig, REAPER and more.

## Free forever, support the culture

MIDI GEN is free, with no trial, no sign-up and nothing locked. Use it on anything you make, including commercial releases.
If it helps your music and you want to keep new plugins coming, hit the **Sponsor** button at the top of this page. Any amount helps.

## Build it yourself

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```
Needs CMake 3.22+ and a C++17 compiler (Xcode on Mac, Visual Studio 2022 on Windows, GCC/Clang on Linux; Linux also needs the ALSA/X11/FreeType dev packages listed in `.github/workflows/build.yml`). JUCE downloads automatically.


## The workflow
1. Drag a chord clip onto a MIDI track. A [HOOKSMITH](https://github.com/Maxswell5412/HOOKSMITH-VST) **Chords** clip is perfect, and so is your own idea or a chord MIDI file from a pack.
2. Put **MIDI GEN** on the same track.
3. Set **LISTEN LENGTH** to the clip length (4, 8 or 16 bars), press **LISTEN**, then play the clip from bar 1. MIDI GEN records one loop and learns the key and the chords. You can also click **LOAD MIDI**, or drag a .mid file onto it.
4. Pick a **STYLE** and hit **GENERATE**.
5. Click a card (A–H) to hear it in place of your clip, in time and with your clip's rhythm. Click **ORIGINAL** to go back. Double-click a card to play it without the DAW.
6. Drag the card you like into Ableton, or use **DRAG ALL 8** to drop all eight back to back as one clip.

## Styles
| Style | What it does |
|---|---|
| Radio Pop | Big and singable: four-chord anthems, sus chords, smooth inversions |
| Moody Rap (OVO) | Late-night sound: minor 9ths, major 7ths, pedal bass, Phrygian colour |
| Dark Trap | Cold and menacing: minor triads, flat-II, harmonic-minor V, diminished tension |
| Soul Ballad | Big-voice ballad: gospel passing chords, secondary dominants, IV to iv, walking bass |
| House / Deep House | Club stabs: minor 7ths and 9ths, two-chord vamps, ii-V lifts |
| Afro / Amapiano | Warm and bouncy: major 7ths, minor 9ths, IV-V-iii-vi movement |
| Neo-Soul / R&B | Lush: 9ths, 11ths, 13ths, tritone subs, chromatic slides |
| Cinematic Pop | Epic: borrowed bVI and bVII, suspensions, open voicings |

The styles are written from the harmony these genres use. They don't copy any song.

## Knobs
- **SIMILARITY**: turned right, it keeps your chords and only recolours them. Turned left, it writes all-new chords that do the same job in the key.
- **COLOR**: plain triads to lush 7ths, 9ths, 11ths and 13ths.
- **SPICE**: passing diminished chords, secondary dominants, sus-to-major resolutions, tritone subs, borrowed iv.
- **BASS MOTION**: inversions for a walking bass line, and pedal bass under the chords.
- **KEEP RHYTHM**: the new chords hit where your clip hit. Turn it off for held chords.
- **ADD BASS**: puts the bass note under each chord.
- Each card shows what it did (e.g. "secondary dominant, inversion"). New chords are **cyan** and kept chords are **amber**.

## Sound
**LOAD** an instrument (VST3/AU), or use the built-in keys. **MIDI OUT** sends the chords out too. Everything (the learned clip, the 8 progressions and your instrument) is saved with your Ableton set.

## Files
`Source/Reharm.*` is the reharmonizer (styles, substitutions, voicing). `Source/PluginProcessor.*` handles listen/learn, playback, hosting and export. `Source/PluginEditor.*` is the interface. `Tests/` contains the automated tests.

## Licence
Free and open source under the **GNU AGPLv3** (see `LICENSE`), built with [JUCE](https://juce.com). The music you make with MIDI GEN is 100% yours.
