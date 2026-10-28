#pragma once

#include <Bela.h>

// Reads a piezo disc on a Bela analog input (e.g. A0).
// One leg to the analog input, the other to GND, 470k-1M resistor across both.
//
// Call Init() in setup() and Read() in render().
class Piezo
{
  public:
    void Init(int channel) { channel_ = channel; }

    // Returns the highest value in this block, 0.0 to 1.0.
    // Hits are very short, so checking every frame catches them.
    float Read(BelaContext* context)
    {
        float peak = 0.f;
        for(unsigned int n = 0; n < context->analogFrames; n++)
        {
            float v = analogRead(context, n, channel_);
            if(v > peak)
                peak = v;
        }
        return peak;
    }

  private:
    int channel_ = 0;
};
