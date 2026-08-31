# RtMidi

![Build Status](https://github.com/thestk/rtmidi/actions/workflows/ci.yml/badge.svg)
[![Conan Center](https://shields.io/conan/v/rtmidi)](https://conan.io/center/rtmidi)

A set of C++ classes that provide a common API for realtime MIDI input/output across Linux (ALSA & JACK), Macintosh OS X (CoreMIDI & JACK), Windows (Multimedia Library & UWP), Web MIDI, iOS and Android.

By Gary P. Scavone, 2003-2023.

This distribution of RtMidi contains the following:

- `doc`:      RtMidi documentation (also online at https://caml.music.mcgill.ca/~gary/rtmidi/)
- `tests`:    example RtMidi programs

On Unix systems, type `./configure` in the top level directory, then `make` in the `tests/` directory to compile the test programs.  In Windows, open the Visual C++ workspace file located in the `tests/` directory.

If you checked out the code from git, please run `./autogen.sh` before `./configure`.

## Overview

RtMidi is a set of C++ classes (`RtMidiIn`, `RtMidiOut`, and API specific classes) that provide a common API (Application Programming Interface) for realtime MIDI input/output across Linux (ALSA, JACK), Macintosh OS X (CoreMIDI, JACK), and Windows (Multimedia Library) operating systems.  RtMidi significantly simplifies the process of interacting with computer MIDI hardware and software.  It was designed with the following goals:

  - object oriented C++ design
  - simple, common API across all supported platforms
  - only one header and one source file for easy inclusion in programming projects
  - MIDI device enumeration

MIDI input and output functionality are separated into two classes, `RtMidiIn` and `RtMidiOut`.  Each class instance supports only a single MIDI connection.  RtMidi does not provide timing functionality (i.e., output messages are sent immediately).  Input messages are timestamped with delta times in seconds (via a `double` floating point type).  MIDI data is passed to the user as raw bytes using an `std::vector<unsigned char>`.

RtMidi is also offered as a module, which is enabled with `RTMIDI_BUILD_MODULES`, and is accessed with `import rt.midi;`. Namespaces are implicitly imported (unless disabled with `RTMIDI_USE_NAMESPACE`), so classes can be accessed through namespace `rt::midi` or through the global namespace (for example, `rt::midi::MidiApi` and `::MidiApi` are both valid).

## Windows

In some cases, for example to use RtMidi with GS Synth, it may be necessary for your program to call `CoInitializeEx` and `CoUninitialize` on entry to and exit from the thread that uses RtMidi.

## WebAssembly / Web MIDI

The CMake build enables the Web MIDI backend by default when RtMidi is configured with the Emscripten toolchain. WebAssembly builds default to a static library.

To build the library only:

```sh
emcmake cmake -S . -B build-wasm \
  -DRTMIDI_BUILD_TESTING=OFF
cmake --build build-wasm
```

The backend can also be selected explicitly with `-DRTMIDI_API_WEBMIDI=ON`. `RtMidiIn` and `RtMidiOut` then expose `RtMidi::WEB_MIDI_API` and use the browser's `navigator.requestMIDIAccess()` API.

### Browser examples

Three browser applications are provided in `tests/`:

- `webmidiprobe`: WebAssembly counterpart of `midiprobe`; enumerates compiled APIs and all available MIDI inputs and outputs.
- `webmidiin`: interactive MIDI input monitor; selects a real Web MIDI input and displays incoming MIDI messages through an RtMidi callback.
- `webmidiout`: interactive MIDI output tester; selects a real Web MIDI output and sends notes from a one-octave keyboard, Control Change messages, and All Notes Off.

Configure and build all three applications with:

```sh
emcmake cmake -S . -B build-wasm \
  -DBUILD_SHARED_LIBS=OFF \
  -DRTMIDI_API_JACK=OFF \
  -DRTMIDI_API_ALSA=OFF \
  -DRTMIDI_API_WEBMIDI=ON \
  -DRTMIDI_BUILD_TESTING=ON

cmake --build build-wasm \
  --target webmidiprobe webmidiin webmidiout
```

Each target produces a complete Emscripten browser bundle in `build-wasm/tests/`:

```text
webmidiprobe.html
webmidiprobe.js
webmidiprobe.wasm

webmidiin.html
webmidiin.js
webmidiin.wasm

webmidiout.html
webmidiout.js
webmidiout.wasm
```

Serve the directory over HTTP:

```sh
python3 -m http.server 8000 --directory build-wasm/tests
```

Then open one of:

```text
http://localhost:8000/webmidiprobe.html
http://localhost:8000/webmidiin.html
http://localhost:8000/webmidiout.html
```

Each application requests Web MIDI access from an explicit browser button and waits for the asynchronous permission request before enumerating ports. `webmidiin` lets you refresh the input list and open a selected port for live monitoring. `webmidiout` lets you refresh the output list, open a selected port, play notes, send CC messages, and send MIDI CC 123 (All Notes Off).

Do not open the generated HTML directly with a `file://` URL: the `.wasm` module is loaded as a separate resource and should be served by an HTTP server. `localhost` is suitable for local development. Web MIDI browser support normally also requires a secure context.

### Headless Web MIDI regression test

The `webmidi-smoke` target uses Node with a mocked Web MIDI implementation to cover API selection, port enumeration, input callback wiring, output messages, and close semantics without requiring physical MIDI hardware:

```sh
cmake --build build-wasm --target webmidi-smoke
ctest --test-dir build-wasm -R webmidi-smoke --output-on-failure
```

## Further reading

For complete documentation on RtMidi, see the `doc` directory of the distribution or surf to https://caml.music.mcgill.ca/~gary/rtmidi/.

## Legal and ethical

The RtMidi license is similar to the MIT License, with the added *feature* that modifications be sent to the developer.  Please see [LICENSE](LICENSE).
