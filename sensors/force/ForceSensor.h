#pragma once

#include <Bela.h>

// Reads a force sensing resistor (FSR) in a voltage divider on a Bela analog
// input (e.g. A2). 3.3V -> FSR -> input -> 10k -> GND, so pressing harder
// gives a higher value.
//
// Call Init() in setup() and Read() in render().
class ForceSensor
{
  public:
    void Init(int channel) { channel_ = channel; }

    // Returns 0.0 (not pressed) to 1.0 (pressed very hard)
    float Read(BelaContext* context)
    {
        if(context->analogFrames == 0)
            return 0.f; // analog inputs are disabled in the project settings
        value_ = analogRead(context, 0, channel_);
        return value_;
    }

    // True while pressed harder than threshold (uses the last Read())
    bool IsPressed(float threshold = 0.05f) const { return value_ > threshold; }

  private:
    int   channel_ = 0;
    float value_   = 0.f;
};
