// webmidiout.cpp
//
// Interactive browser MIDI output tester for the Web MIDI backend.

#include "RtMidi.h"

#include <emscripten.h>

#include <iostream>
#include <memory>
#include <string>

using namespace rt::midi;

namespace {

std::unique_ptr<RtMidiOut> midiOut;
std::string portNameBuffer;

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

bool validDataByte(int value)
{
  return value >= 0 && value <= 127;
}

int sendThreeBytes(unsigned char status,
                   unsigned char data1,
                   unsigned char data2)
{
  if (!midiOut || !midiOut->isPortOpen())
    return 1;

  const unsigned char message[] = { status, data1, data2 };
  try {
    midiOut->sendMessage(message, sizeof(message));
  }
  catch (RtMidiError &error) {
    error.printMessage();
    return 2;
  }
  return 0;
}

} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE int webmidiout_access_state()
{
  return accessState();
}

extern "C" EMSCRIPTEN_KEEPALIVE int webmidiout_init()
{
  if (midiOut)
    return 0;

  try {
    midiOut.reset(new RtMidiOut(RtMidi::WEB_MIDI_API,
                                "RtMidi Web MIDI Output Tester"));
  }
  catch (RtMidiError &error) {
    error.printMessage();
    midiOut.reset();
    return 1;
  }

  std::cout << "Web MIDI output access requested." << std::endl;
  return 0;
}

extern "C" EMSCRIPTEN_KEEPALIVE int webmidiout_port_count()
{
  if (!midiOut || accessState() != 2)
    return 0;

  try {
    return static_cast<int>(midiOut->getPortCount());
  }
  catch (RtMidiError &error) {
    error.printMessage();
    return 0;
  }
}

extern "C" EMSCRIPTEN_KEEPALIVE const char *webmidiout_port_name(int port)
{
  portNameBuffer.clear();
  if (!midiOut || accessState() != 2 || port < 0)
    return portNameBuffer.c_str();

  try {
    const unsigned int index = static_cast<unsigned int>(port);
    if (index >= midiOut->getPortCount())
      return portNameBuffer.c_str();
    portNameBuffer = midiOut->getPortName(index);
  }
  catch (RtMidiError &error) {
    error.printMessage();
  }

  return portNameBuffer.c_str();
}

extern "C" EMSCRIPTEN_KEEPALIVE int webmidiout_open(int port)
{
  if (!midiOut || accessState() != 2 || port < 0)
    return 1;

  try {
    const unsigned int index = static_cast<unsigned int>(port);
    if (index >= midiOut->getPortCount())
      return 2;

    if (midiOut->isPortOpen())
      midiOut->closePort();

    midiOut->openPort(index);
    std::cout << "Opened MIDI output #" << index << ": "
              << midiOut->getPortName(index) << std::endl;
  }
  catch (RtMidiError &error) {
    error.printMessage();
    return 3;
  }

  return 0;
}

extern "C" EMSCRIPTEN_KEEPALIVE void webmidiout_close()
{
  if (!midiOut || !midiOut->isPortOpen())
    return;

  midiOut->closePort();
  std::cout << "MIDI output closed." << std::endl;
}

extern "C" EMSCRIPTEN_KEEPALIVE int webmidiout_note_on(int channel,
                                                         int note,
                                                         int velocity)
{
  if (channel < 1 || channel > 16 || !validDataByte(note) ||
      !validDataByte(velocity))
    return 3;

  return sendThreeBytes(static_cast<unsigned char>(0x90 | (channel - 1)),
                        static_cast<unsigned char>(note),
                        static_cast<unsigned char>(velocity));
}

extern "C" EMSCRIPTEN_KEEPALIVE int webmidiout_note_off(int channel,
                                                          int note,
                                                          int velocity)
{
  if (channel < 1 || channel > 16 || !validDataByte(note) ||
      !validDataByte(velocity))
    return 3;

  return sendThreeBytes(static_cast<unsigned char>(0x80 | (channel - 1)),
                        static_cast<unsigned char>(note),
                        static_cast<unsigned char>(velocity));
}

extern "C" EMSCRIPTEN_KEEPALIVE int webmidiout_cc(int channel,
                                                    int controller,
                                                    int value)
{
  if (channel < 1 || channel > 16 || !validDataByte(controller) ||
      !validDataByte(value))
    return 3;

  return sendThreeBytes(static_cast<unsigned char>(0xB0 | (channel - 1)),
                        static_cast<unsigned char>(controller),
                        static_cast<unsigned char>(value));
}

extern "C" EMSCRIPTEN_KEEPALIVE int webmidiout_all_notes_off(int channel)
{
  if (channel < 1 || channel > 16)
    return 3;

  return sendThreeBytes(static_cast<unsigned char>(0xB0 | (channel - 1)),
                        123,
                        0);
}

int main()
{
  std::cout << "RtMidi Web MIDI Output Tester loaded." << std::endl;
  return 0;
}
