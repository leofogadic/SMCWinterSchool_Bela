#pragma once

#include <Bela.h>

// Reads a potentiometer on a Bela analog input (e.g. A0).
// Outer legs to 3.3V and GND, wiper to the analog input.
//
// Call Init() in setup() and Read() in render().
class Potentiometer
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
