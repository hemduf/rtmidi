// RtMidiWeb.cpp
//
// Web MIDI input-dispatch fixes kept separate from the historical backend in
// RtMidi.cpp. The CMake Web MIDI build renames only the legacy free callback
// symbol, allowing this translation unit to provide corrected dispatch without
// changing the RtMidi class definition in any translation unit.

#include "RtMidi.h"

#if defined(__WEB_MIDI_API__)

#include <emscripten.h>

#include <cstdint>
#include <iostream>

using namespace rt::midi;

namespace {

bool shouldIgnore(const MidiInApi::RtMidiInData &data,
                  const std::uint8_t *bytes,
                  std::int32_t length)
{
  if (!bytes || length <= 0)
    return true;

  const unsigned char status = bytes[0];

  if (status == 0xF0 && (data.ignoreFlags & 0x01))
    return true;

  if ((status == 0xF1 || status == 0xF8) && (data.ignoreFlags & 0x02))
    return true;

  if (status == 0xFE && (data.ignoreFlags & 0x04))
    return true;

  return false;
}

} // namespace

extern "C" void EMSCRIPTEN_KEEPALIVE rtmidi_onMidiMessageProc(
    MidiInApi::RtMidiInData *data,
    std::uint8_t *inputBytes,
    std::int32_t length,
    double deltaMilliseconds)
{
  if (!data || !inputBytes || length <= 0)
    return;

  MidiInApi::MidiMessage &message = data->message;
  const double deltaSeconds = deltaMilliseconds * 0.001;

  // RtMidi timestamps measure time since the previous message delivered to the
  // user. Keep accumulating deltas while filtered events are skipped. If all
  // events before the first delivered message are filtered, the first visible
  // message still correctly receives a timestamp of zero.
  if (shouldIgnore(*data, inputBytes, length)) {
    if (!data->firstMessage)
      message.timeStamp += deltaSeconds;
    return;
  }

  if (data->firstMessage) {
    data->firstMessage = false;
    message.timeStamp = 0.0;
  }
  else {
    message.timeStamp += deltaSeconds;
  }

  message.bytes.assign(inputBytes, inputBytes + length);

  if (data->usingCallback) {
    RtMidiIn::RtMidiCallback callback =
        reinterpret_cast<RtMidiIn::RtMidiCallback>(data->userCallback);
    callback(message.timeStamp, &message.bytes, data->userData);
  }
  else if (data->queue.ringSize > 0) {
    if (!data->queue.push(message))
      std::cerr << "MidiInWeb: message queue limit reached!!" << std::endl;
  }

  // The queue stores a copy and callbacks must observe exactly one message.
  message.bytes.clear();
  message.timeStamp = 0.0;
}

#endif // __WEB_MIDI_API__
