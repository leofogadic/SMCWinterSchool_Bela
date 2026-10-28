#pragma once

#include <Bela.h>

// Reads an Adafruit conductive rubber cord stretch sensor in a voltage
// divider on a Bela analog input (e.g. A3).
// 3.3V -> cord -> input -> resistor -> GND: stretching lowers the value.
// Swap the cord and the resistor to make stretching raise it instead.
//
// Call Init() in setup() and Read() in render().
class StretchSensor
{
  public:
    void Init(int channel) { channel_ = channel; }

    // Returns 0.0 to 1.0
    float Read(BelaContext* context)
    {
        if(context->analogFrames == 0)
            return 0.f; // analog inputs are disabled in the project settings
        return analogRead(context, 0, channel_);
    }

  private:
    int channel_ = 0;
};
