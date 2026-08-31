// RtMidiWeb.cpp
//
// Web MIDI compatibility fixes kept separate from the platform-neutral
// RtMidi.cpp implementation. This translation unit replaces the legacy Web
// MIDI callback and public compiled-API enumeration when Web MIDI is enabled.

#include "RtMidi.h"
#include "rtmidi_c.h"

#if defined(__WEB_MIDI_API__)

#include <emscripten.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace rt::midi;

extern "C" {
extern const RtMidi::Api rtmidi_compiled_apis[];
extern const unsigned int rtmidi_num_compiled_apis;
}

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

void RtMidi::getCompiledApi(std::vector<RtMidi::Api> &apis) throw()
{
  apis.clear();
  apis.reserve(rtmidi_num_compiled_apis);

  for (unsigned int i = 0; i < rtmidi_num_compiled_apis; ++i) {
    const RtMidi::Api api = rtmidi_compiled_apis[i];
    if (std::find(apis.begin(), apis.end(), api) == apis.end())
      apis.push_back(api);
  }
}

extern "C" int rtmidi_get_compiled_api(enum RtMidiApi *apis,
                                        unsigned int apis_size)
{
  std::vector<RtMidi::Api> compiled;
  RtMidi::getCompiledApi(compiled);

  if (!apis)
    return static_cast<int>(compiled.size());

  const unsigned int count = std::min<unsigned int>(
      apis_size, static_cast<unsigned int>(compiled.size()));

  for (unsigned int i = 0; i < count; ++i)
    apis[i] = static_cast<RtMidiApi>(compiled[i]);

  return static_cast<int>(count);
}

extern "C" void EMSCRIPTEN_KEEPALIVE rtmidi_onMidiMessageProc(
    MidiInApi::RtMidiInData *data,
    std::uint8_t *inputBytes,
    std::int32_t length,
    double deltaMilliseconds)
{
  if (!data || shouldIgnore(*data, inputBytes, length))
    return;

  MidiInApi::MidiMessage &message = data->message;
  message.bytes.assign(inputBytes, inputBytes + length);
  message.timeStamp = deltaMilliseconds * 0.001;

  if (data->usingCallback) {
    RtMidiIn::RtMidiCallback callback =
        reinterpret_cast<RtMidiIn::RtMidiCallback>(data->userCallback);
    callback(message.timeStamp, &message.bytes, data->userData);
  }
  else if (data->queue.ringSize > 0) {
    if (!data->queue.push(message))
      std::cerr << "MidiInWeb: message queue limit reached!!" << std::endl;
  }

  // The queue stores a copy and callbacks must observe one message at a time.
  // Never let the internal scratch vector leak bytes into the next event.
  message.bytes.clear();
}

#endif // __WEB_MIDI_API__
