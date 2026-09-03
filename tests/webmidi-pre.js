(function () {
  var input = {
    name: "Mock Input",
    onmidimessage: null
  };

  var output = {
    name: "Mock Output",
    sent: [],
    send: function (message) {
      this.sent.push(Array.prototype.slice.call(message));
    }
  };

  var access = {
    inputs: new Map([["input-0", input]]),
    outputs: new Map([["output-0", output]])
  };

  var navigatorMock = {
    requestMIDIAccess: function (options) {
      globalThis.__rtmidiRequestedSysex = !!(options && options.sysex);

      // RtMidi exposes synchronous port enumeration while Web MIDI grants
      // access asynchronously. A synchronous thenable keeps this test
      // deterministic while exercising the production bridge unchanged.
      return {
        then: function (resolve) {
          resolve(access);
          return this;
        }
      };
    }
  };

  globalThis.window = globalThis;
  Object.defineProperty(globalThis, "navigator", {
    value: navigatorMock,
    configurable: true
  });
  globalThis.__rtmidiTestAccess = access;
}());
