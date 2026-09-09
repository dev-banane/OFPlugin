# OFPlugin

A EuroScope plugin that shows each aircraft's current radio frequency right
in its tag, so you can see who's on which frequency at a glance.

## What it does

OFPlugin adds a "Pilot frequency" tag item. Once you add it to your tag
layout, every aircraft on your screen shows the frequency its pilot is
tuned to, like `128.725`.

Frequencies come from two sources:

- **VATSIM's data feed.** Updated about every 15 seconds, covering every
  connected pilot. This is a good guess, but pilots with two radios
  sometimes have it slightly wrong.
- **TrackAudio, if you use it.** If a pilot actually transmits to you,
  OFPlugin knows their frequency for certain and locks it in. That locked-in
  value stays even if the feed guesses something else, and only clears once
  the aircraft has been gone from your screen for about a minute. No setup
  needed. If you don't use TrackAudio, this part is simply skipped.

Only aircraft currently shown on your screen are looked up. OFPlugin never
tracks or stores anything for traffic you can't see.

## Installing

1. Download or build `OFPlugin.dll` (see below).
2. In EuroScope, go to **Other Set Files > Plug-ins...**
3. Click **Load** and select `OFPlugin.dll`.
4. Open your tag layout editor and add the "Pilot frequency" item wherever
   you'd like it to show up.

That's it. Frequencies should start appearing within about 20 seconds of
connecting.

## Requirements

- EuroScope (Windows). The plugin is 32-bit only, matching EuroScope itself.
- An internet connection, to reach the VATSIM data feed.
- TrackAudio is optional. It only improves accuracy for pilots who transmit
  to your frequency; everything else works without it.

## Building from source

Requires CMake and MSVC (Visual Studio Build Tools or Visual Studio, C++
workload). The build must target 32-bit (Win32); EuroScope cannot load a
64-bit plugin.

```bash
cmake -S . -B build -A Win32
cmake --build build --config Debug
```

The DLL ends up at `build/Debug/OFPlugin.dll`.

## For developers

- [`src/OFPlugIn.h`](src/OFPlugIn.h) / [`.cpp`](src/OFPlugIn.cpp): the
  `COFPlugIn` class. Registers the tag item and answers tag requests with a
  plain cache lookup. No network calls happen on EuroScope's own thread.
- [`src/TransceiverFetcher.h`](src/TransceiverFetcher.h) /
  [`.cpp`](src/TransceiverFetcher.cpp): polls
  `https://data.vatsim.net/v3/transceivers-data.json` on a background thread
  and picks a best-guess frequency per callsign. A callsign can list several
  transceivers (dual-COM pilots, multi-radio ATC positions), and the
  transceiver's `id` field turned out not to reliably mark which one is
  "primary" (checked against a live sample of the feed, it matched only
  about half the time). So the picker prefers the first frequency that
  isn't 121.500 (guard) or 122.800 (UNICOM), falling back to guard/UNICOM
  only if nothing else is present.
- [`src/TrackAudioClient.h`](src/TrackAudioClient.h) /
  [`.cpp`](src/TrackAudioClient.cpp): connects to
  [TrackAudio](https://github.com/pierr3/TrackAudio)'s local WebSocket API
  (`ws://127.0.0.1:49080/ws`) and listens for `kRxBegin` events. TrackAudio
  only reports transmissions on frequencies it's configured with, which are
  the controller's own working frequencies, so a callsign heard there is a
  confirmed COM1, not a guess. Fails to connect silently if TrackAudio isn't
  running, and retries every 10 seconds.
- [`src/FrequencyStore.h`](src/FrequencyStore.h) /
  [`.cpp`](src/FrequencyStore.cpp): mutex-guarded state shared between the
  two background threads and EuroScope's main thread. Confirmed (TrackAudio)
  frequencies take priority over feed guesses and expire 60 seconds after
  the callsign stops being displayed.
- [`vendor/euroscope-sdk/`](vendor/euroscope-sdk/): vendored EuroScope SDK
  (`EuroScopePlugIn.h` + `EuroScopePlugInDll.lib`). Not published on any
  package registry; pulled from the
  [FeronFae/EuroScopePlugInSDK](https://github.com/FeronFae/EuroScopePlugInSDK)
  mirror. If the plugin API ever seems to drift, diff these against the
  copies shipped with a real, current EuroScope install
  (`%appdata%\EuroScope\PlugIn\`) and update accordingly.
- [`vendor/nlohmann-json/`](vendor/nlohmann-json/): vendored single-header
  JSON parser ([nlohmann/json](https://github.com/nlohmann/json), MIT).

Two compiler settings in [`CMakeLists.txt`](CMakeLists.txt) are required and
easy to accidentally undo:

- `_MBCS`, not `_UNICODE`. Neither FSD nor EuroScope handle Unicode strings.
- `/Zc:wchar_t-`, for ABI compatibility with the EuroScope SDK.
