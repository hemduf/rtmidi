// webmidiprobe.cpp
//
// Browser-based MIDI input/output probe for the Web MIDI backend.
//
// This program is built with Emscripten and driven by webmidiprobe.html.
// Web MIDI permission is asynchronous, so the HTML shell requests access,
// waits for it to settle, then calls rtmidi_web_probe().

#include "RtMidi.h"

#include <emscripten.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace rt::midi;

namespace {

std::unique_ptr<RtMidiIn> midiIn;
std::unique_ptr<RtMidiOut> midiOut;

void printApi(RtMidi::Api api)
{
  std::string displayName = RtMidi::getApiDisplayName(api);
  if (displayName.empty())
    displayName = RtMidi::getApiName(api);

  std::cout << "  " << displayName << std::endl;
}

} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE int rtmidi_web_access_state()
{
  //  0: Web MIDI supported, access not requested yet
  //  1: permission request pending
  //  2: MIDI access ready
  // -1: permission denied / access unavailable after request
  // -2: browser does not expose navigator.requestMIDIAccess()
  return EM_ASM_INT({
    if (typeof navigator === 'undefined' ||
        typeof navigator.requestMIDIAccess !== 'function')
      return -2;

    if (typeof globalThis._rtmidi_internals_waiting === 'undefined')
      return 0;

    if (globalThis._rtmidi_internals_waiting)
      return 1;

    if (typeof globalThis._rtmidi_internals_midi_access === 'undefined' ||
        globalThis._rtmidi_internals_midi_access === null)
      return -1;

    return 2;
  });
}

extern "C" EMSCRIPTEN_KEEPALIVE int rtmidi_web_init()
{
  if (midiIn && midiOut)
    return 0;

  try {
    midiIn.reset(new RtMidiIn(RtMidi::WEB_MIDI_API,
                              "RtMidi Web MIDI Probe Input"));
    midiOut.reset(new RtMidiOut(RtMidi::WEB_MIDI_API,
                                "RtMidi Web MIDI Probe Output"));
  }
  catch (RtMidiError &error) {
    error.printMessage();
    midiIn.reset();
    midiOut.reset();
    return 1;
  }

  std::cout << "Web MIDI access requested. Waiting for browser permission..."
            << std::endl;
  return 0;
}

extern "C" EMSCRIPTEN_KEEPALIVE int rtmidi_web_probe()
{
  if (!midiIn || !midiOut) {
    std::cerr << "Web MIDI has not been initialized." << std::endl;
    return 1;
  }

  if (rtmidi_web_access_state() != 2) {
    std::cerr << "Web MIDI access is not ready." << std::endl;
    return 2;
  }

  try {
    std::vector<RtMidi::Api> apis;
    RtMidi::getCompiledApi(apis);

    std::cout << "\nCompiled APIs:" << std::endl;
    for (std::vector<RtMidi::Api>::const_iterator it = apis.begin();
         it != apis.end(); ++it)
      printApi(*it);

    std::cout << "\nCurrent input API: "
              << RtMidi::getApiDisplayName(midiIn->getCurrentApi())
              << std::endl;

    unsigned int nPorts = midiIn->getPortCount();
    std::cout << "There are " << nPorts
              << " MIDI input sources available." << std::endl;
    for (unsigned int i = 0; i < nPorts; ++i)
      std::cout << "  Input Port #" << i << ": "
                << midiIn->getPortName(i) << std::endl;

    std::cout << "\nCurrent output API: "
              << RtMidi::getApiDisplayName(midiOut->getCurrentApi())
              << std::endl;

    nPorts = midiOut->getPortCount();
    std::cout << "There are " << nPorts
              << " MIDI output ports available." << std::endl;
    for (unsigned int i = 0; i < nPorts; ++i)
      std::cout << "  Output Port #" << i << ": "
                << midiOut->getPortName(i) << std::endl;

    std::cout << std::endl;
  }
  catch (RtMidiError &error) {
    error.printMessage();
    return 3;
  }

  return 0;
}

int main()
{
  std::cout << "RtMidi Web MIDI Probe loaded." << std::endl;
  std::cout << "Use the browser controls to request MIDI access and probe ports."
            << std::endl;
  return 0;
}
