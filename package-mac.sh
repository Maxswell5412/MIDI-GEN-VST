#!/bin/bash
# Builds MIDI GEN for Intel + Apple Silicon Macs and makes MIDIGEN-Mac.zip
# (ready to email) in your Downloads folder.  Run:  bash package-mac.sh
set -e
cd "$(dirname "$0")"
CMAKE=cmake; command -v cmake >/dev/null || CMAKE=/Applications/CMake.app/Contents/bin/cmake
$CMAKE -B build-dist -DCMAKE_BUILD_TYPE=Release -DMG_COPY_AFTER_BUILD=OFF \
      -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
$CMAKE --build build-dist --config Release -j8 --target MidiGen_AU MidiGen_VST3
A=build-dist/MidiGen_artefacts/Release
codesign --force --deep --sign - "$A/AU/MIDI GEN.component"
codesign --force --deep --sign - "$A/VST3/MIDI GEN.vst3"
OUT=build-dist/MIDIGEN-Mac
rm -rf "$OUT" && mkdir -p "$OUT"
cp -R "$A/AU/MIDI GEN.component" "$A/VST3/MIDI GEN.vst3" "$OUT/"
cp "Install/Install on Mac.txt" "$OUT/"
rm -f ~/Downloads/MIDIGEN-Mac.zip
ditto -c -k --keepParent "$OUT" ~/Downloads/MIDIGEN-Mac.zip
lipo -archs "$A/AU/MIDI GEN.component/Contents/MacOS/MIDI GEN"
echo "Done: ~/Downloads/MIDIGEN-Mac.zip"
