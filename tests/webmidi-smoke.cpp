#include "RtMidi.h"

#include <emscripten.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

struct ReceivedMessage {
  ReceivedMessage(double timestamp,
                  const std::vector<unsigned char>& messageBytes)
    : timeStamp(timestamp), bytes(messageBytes) {}

  double timeStamp;
  std::vector<unsigned char> bytes;
};

std::vector<ReceivedMessage> received;

void midiCallback(double timeStamp,
                  std::vector<unsigned char>* message,
                  void*)
{
  if (message)
    received.push_back(ReceivedMessage(timeStamp, *message));
}

bool equals(const std::vector<unsigned char>& actual,
            const unsigned char* expected,
            std::size_t size)
{
  return actual.size() == size &&
         std::equal(actual.begin(), actual.end(), expected);
}

void inject(unsigned char status,
            unsigned char data1,
            unsigned char data2,
            int size,
            double timestamp)
{
  EM_ASM({
    var mockInput = globalThis.__rtmidiTestAccess.inputs.get("input-0");
    if (!mockInput.onmidimessage)
      throw new Error("RtMidi did not install the Web MIDI input callback");
    var bytes = [$0, $1, $2].slice(0, $3);
    mockInput.onmidimessage({
      data: new Uint8Array(bytes),
      timeStamp: $4
    });
  }, status, data1, data2, size, timestamp);
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

  const unsigned char noteOn[] = {0x90, 60, 100};
  const unsigned char noteOff[] = {0x80, 60, 0};
  const unsigned char clock[] = {0xF8};

  inject(noteOn[0], noteOn[1], noteOn[2], 3, 1000.0);
  inject(noteOff[0], noteOff[1], noteOff[2], 3, 1012.5);

  if (received.size() != 2)
    return 7;
  if (!equals(received[0].bytes, noteOn, sizeof(noteOn)))
    return 8;
  if (!equals(received[1].bytes, noteOff, sizeof(noteOff)))
    return 9;
  if (std::fabs(received[0].timeStamp) > 1.0e-12)
    return 10;
  if (std::fabs(received[1].timeStamp - 0.0125) > 1.0e-9)
    return 11;

  // Timing messages are ignored by default. Their elapsed time must be added
  // to the next message that is actually delivered to the user.
  inject(clock[0], 0, 0, 1, 1020.0);
  if (received.size() != 2)
    return 12;

  input.ignoreTypes(false, false, false);
  inject(clock[0], 0, 0, 1, 1030.0);
  if (received.size() != 3 || !equals(received.back().bytes, clock, 1))
    return 13;
  if (std::fabs(received.back().timeStamp - 0.0175) > 1.0e-9)
    return 14;

  input.closePort();
  if (input.isPortOpen())
    return 15;

  // With no user callback, Web MIDI must feed the normal RtMidi queue. The
  // first delivered message of a fresh input instance has a zero delta time.
  RtMidiIn queuedInput(RtMidi::WEB_MIDI_API);
  queuedInput.openPort(0);
  inject(noteOn[0], 64, 90, 3, 1040.0);

  std::vector<unsigned char> queuedMessage;
  const double queuedTimeStamp = queuedInput.getMessage(&queuedMessage);
  const unsigned char queuedExpected[] = {0x90, 64, 90};
  if (!equals(queuedMessage, queuedExpected, sizeof(queuedExpected)))
    return 16;
  if (std::fabs(queuedTimeStamp) > 1.0e-12)
    return 17;
  queuedInput.closePort();

  RtMidiOut output(RtMidi::WEB_MIDI_API);
  if (output.getCurrentApi() != RtMidi::WEB_MIDI_API)
    return 18;
  if (output.getPortCount() != 1)
    return 19;
  if (output.getPortName(0) != "Mock Output")
    return 20;

  output.openPort(0);
  if (!output.isPortOpen())
    return 21;

  const unsigned char cc[] = {0xB0, 1, 64};
  output.sendMessage(noteOff, sizeof(noteOff));
  output.sendMessage(cc, sizeof(cc));

  if (!EM_ASM_INT({
        var sent = globalThis.__rtmidiTestAccess.outputs.get("output-0").sent;
        return sent && sent.length === 2 &&
               sent[0].length === 3 &&
               sent[0][0] === 0x80 && sent[0][1] === 60 && sent[0][2] === 0 &&
               sent[1].length === 3 &&
               sent[1][0] === 0xB0 && sent[1][1] === 1 && sent[1][2] === 64 ? 1 : 0;
      }))
    return 22;

  output.closePort();
  if (output.isPortOpen())
    return 23;

  return 0;
}
