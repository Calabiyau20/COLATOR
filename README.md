# Percoolator

Advanced snare and percussion design instrument, by **Calabi Yau**.

Three sound engines, six processors in series, a fast parallel VCA, and eight
retrigger random modulators you assign by dragging them onto any knob.

```
Body + Noise + Snap -> Transient -> Resonator -> Space -> Diode Clip -> VCA (parallel) -> Mixer -> Out
```

The Diode Clip sits **after** Space on purpose, so it bites the delay and reverb
tails too. The VCA runs in parallel: its compressed copy is added to the chain,
never swapped in. The Mixer closes by blending the processed chain against the
raw Body + Noise + Snap sum tapped before any of it.

## Downloads

Every push builds Windows and macOS. Grab the zip from the newest run under
[Actions](../../actions) — each one carries its own `INSTALL.txt`.

- **macOS** — universal (Apple Silicon + Intel): VST3, AU, Standalone
- **Windows** — VST3 and Standalone

## Build from source

macOS: double-click `build.command`. It fetches JUCE, builds Release, and
installs into your plugin folders.

Any platform:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

JUCE 7.0.12 is fetched automatically. On Xcode 15.0 exactly, the build switches
to the classic linker to work around Apple's known linker bug in that release.

## Licence note

`JUCE_DISPLAY_SPLASH_SCREEN=0` requires a JUCE Personal or Indie licence. Set it
to 1 in `CMakeLists.txt` to build under the GPL terms instead.
