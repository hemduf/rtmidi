#include "RtMidi.h"

#include <emscripten.h>

#include <algorithm>
#include <vector>

namespace {

bool received = false;

void midiCallback(double,
                  std::vector<unsigned char>* message,
                  void*)
{
  const std::vector<unsigned char> expected{0x90, 60, 100};
  received = message != nullptr && *message == expected;
}

} // namespace

int main()
{
  using rt::midi::RtMidi;
  using rt::midi::RtMidiIn;
  using rt::midi::RtMidiOut;

  std::vector<RtMidi::Api> apis;
  RtMidi::getCompiledApi(apis);
  if (std::find(apis.begin(), apis.end(), RtMidi::WEB_MIDI_API) == apis.end())
    return 1;

  RtMidiIn input(RtMidi::WEB_MIDI_API);
  if (input.getCurrentApi() != RtMidi::WEB_MIDI_API)
    return 2;
  if (input.getPortCount() != 1)
    return 3;
  if (input.getPortName(0) != "Mock Input")
    return 4;
  if (!EM_ASM_INT({ return globalThis.__rtmidiRequestedSysex ? 1 : 0; }))
    return 5;

  input.setCallback(&midiCallback);
  input.openPort(0);
  if (!input.isPortOpen())
    return 6;

  EM_ASM({
    var mockInput = globalThis.__rtmidiTestAccess.inputs.get("input-0");
    if (!mockInput.onmidimessage)
      throw new Error("RtMidi did not install the Web MIDI input callback");
    mockInput.onmidimessage({
      data: new Uint8Array([0x90, 60, 100]),
      timeStamp: 1000.0
    });
  });

  if (!received)
    return 7;

  RtMidiOut output(RtMidi::WEB_MIDI_API);
  if (output.getCurrentApi() != RtMidi::WEB_MIDI_API)
    return 8;
  if (output.getPortCount() != 1)
    return 9;
  if (output.getPortName(0) != "Mock Output")
    return 10;

  output.openPort(0);
  if (!output.isPortOpen())
    return 11;

  const unsigned char noteOff[] = {0x80, 60, 0};
  output.sendMessage(noteOff, sizeof(noteOff));

  if (!EM_ASM_INT({
        var sent = globalThis.__rtmidiTestAccess.outputs.get("output-0").sent;
        return sent &&
               sent.length === 3 &&
               sent[0] === 0x80 &&
               sent[1] === 60 &&
               sent[2] === 0 ? 1 : 0;
      }))
    return 12;

  input.closePort();
  output.closePort();
  if (input.isPortOpen() || output.isPortOpen())
    return 13;

  return 0;
}
