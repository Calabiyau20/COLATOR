#!/bin/bash
# Percoolator — build VST3 + AU (universal) for macOS and install them.
cd "$(dirname "$0")"
LOG="$(pwd)/build.log"
TAG="juce7012-universal-v2"
{
  echo "== Percoolator build $(date) =="
  if ! xcode-select -p >/dev/null 2>&1; then echo "XCODE_MISSING"; exit 1; fi
  if ! command -v cmake >/dev/null 2>&1; then
    if command -v brew >/dev/null 2>&1; then brew install cmake || exit 1;
    else echo "CMAKE_MISSING"; exit 1; fi
  fi
  if [ -f build/.tag ] && [ "$(cat build/.tag)" = "$TAG" ]; then
    echo "-- reusing build tree"
  else
    echo "-- wiping stale build tree"; rm -rf build
  fi
  cmake -B build -DCMAKE_BUILD_TYPE=Release || { echo "CONFIGURE_FAILED"; exit 1; }
  echo "$TAG" > build/.tag
  cmake --build build --config Release -j"$(sysctl -n hw.ncpu)" || { echo "BUILD_FAILED"; exit 1; }

  # the old name would otherwise sit beside it in the plugin folders
  rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/Calabi Snare.vst3" \
         "$HOME/Library/Audio/Plug-Ins/Components/Calabi Snare.component" 2>/dev/null

  echo "== Installed =="
  ls -d "$HOME/Library/Audio/Plug-Ins/VST3/Percoolator.vst3" 2>/dev/null
  ls -d "$HOME/Library/Audio/Plug-Ins/Components/Percoolator.component" 2>/dev/null
  echo "== Architectures =="
  file "$HOME/Library/Audio/Plug-Ins/VST3/Percoolator.vst3/Contents/MacOS/Percoolator" 2>/dev/null
  echo "BUILD_OK"
} > "$LOG" 2>&1
echo
tail -8 "$LOG"
echo
echo "Full log: $LOG"
