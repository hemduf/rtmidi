// webmidiin.cpp
//
// Interactive browser MIDI input monitor for the Web MIDI backend.

#include "RtMidi.h"

#include <emscripten.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace rt::midi;

namespace {

std::unique_ptr<RtMidiIn> midiIn;
std::string portNameBuffer;

void midiCallback(double /*deltaTime*/,
                  std::vector<unsigned char> *message,
                  void *)
{
  if (!message || message->empty())
    return;

  std::ostringstream out;
  out << "MIDI IN  bytes:";

  for (std::vector<unsigned char>::const_iterator it = message->begin();
       it != message->end(); ++it) {
    out << " " << std::hex << std::uppercase << std::setw(2)
        << std::setfill('0') << static_cast<unsigned int>(*it);
  }

  std::cout << out.str() << std::endl;

  // The current Web MIDI backend reuses its internal message vector.
  // Clearing it here keeps this monitor focused on the current event.
  message->clear();
}

int accessState()
{
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

} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE int webmidiin_access_state()
{
  return accessState();
}

extern "C" EMSCRIPTEN_KEEPALIVE int webmidiin_init()
{
  if (midiIn)
    return 0;

  try {
    midiIn.reset(new RtMidiIn(RtMidi::WEB_MIDI_API,
                              "RtMidi Web MIDI Input Monitor"));
    midiIn->ignoreTypes(false, false, false);
    midiIn->setCallback(&midiCallback);
  }
  catch (RtMidiError &error) {
    error.printMessage();
    midiIn.reset();
    return 1;
  }

  std::cout << "Web MIDI input access requested." << std::endl;
  return 0;
}

extern "C" EMSCRIPTEN_KEEPALIVE int webmidiin_port_count()
{
  if (!midiIn || accessState() != 2)
    return 0;

  try {
    return static_cast<int>(midiIn->getPortCount());
  }
  catch (RtMidiError &error) {
    error.printMessage();
    return 0;
  }
}

extern "C" EMSCRIPTEN_KEEPALIVE const char *webmidiin_port_name(int port)
{
  portNameBuffer.clear();
  if (!midiIn || accessState() != 2 || port < 0)
    return portNameBuffer.c_str();

  try {
    const unsigned int index = static_cast<unsigned int>(port);
    if (index >= midiIn->getPortCount())
      return portNameBuffer.c_str();
    portNameBuffer = midiIn->getPortName(index);
  }
  catch (RtMidiError &error) {
    error.printMessage();
  }

  return portNameBuffer.c_str();
}

extern "C" EMSCRIPTEN_KEEPALIVE int webmidiin_open(int port)
{
  if (!midiIn || accessState() != 2 || port < 0)
    return 1;

  try {
    const unsigned int index = static_cast<unsigned int>(port);
    if (index >= midiIn->getPortCount())
      return 2;

    if (midiIn->isPortOpen())
      midiIn->closePort();

    midiIn->openPort(index);
    std::cout << "Opened MIDI input #" << index << ": "
              << midiIn->getPortName(index) << std::endl;
  }
  catch (RtMidiError &error) {
    error.printMessage();
    return 3;
  }

  return 0;
}

extern "C" EMSCRIPTEN_KEEPALIVE void webmidiin_close()
{
  if (!midiIn || !midiIn->isPortOpen())
    return;

  midiIn->closePort();
  std::cout << "MIDI input closed." << std::endl;
}

int main()
{
  std::cout << "RtMidi Web MIDI Input Monitor loaded." << std::endl;
  return 0;
}
