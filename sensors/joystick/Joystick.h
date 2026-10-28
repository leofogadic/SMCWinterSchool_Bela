#pragma once

#include <Bela.h>
#include "../button/Button.h"

// Reads an X/Y joystick with a push switch (e.g. KY-023 module) on Bela.
// VRx and VRy go to analog inputs, SW goes to a digital pin with a
// 10k pull-up to 3.3V (SW connects to GND when pressed).
//
// Call Init() in setup() and Update() once at the start of every render().
// Leave the stick centered when the program starts: the first reading
// is used as the middle position.
class Joystick
{
  public:
    // The push switch (all Button functions work: sw.JustPressed(), ...)
    Button sw;

    void Init(BelaContext* context, int x_channel, int y_channel, int sw_pin)
    {
        x_channel_ = x_channel;
        y_channel_ = y_channel;
        sw.Init(context, sw_pin, true);
    }

    void Update(BelaContext* context)
    {
        sw.Update(context);
        if(context->analogFrames == 0)
            return; // analog inputs are disabled in the project settings

        raw_x_ = analogRead(context, 0, x_channel_);
        raw_y_ = analogRead(context, 0, y_channel_);

        // First reading = middle position
        if(!centered_)
        {
            center_x_ = raw_x_;
            center_y_ = raw_y_;
            centered_ = true;
        }
    }

    // -1.0 to 1.0, 0 in the middle
    float X() const { return Center(raw_x_, center_x_); }
    float Y() const { return Center(raw_y_, center_y_); }

    // Straight from the analog input
    float RawX() const { return raw_x_; }
    float RawY() const { return raw_y_; }

    // True while the stick is pressed down
    bool IsPressed() const { return sw.IsPressed(); }

  private:
    // Values closer than this to the middle count as 0 (stops jitter at rest)
    static constexpr float kDeadzone = 0.05f;

    int   x_channel_ = 0;
    int   y_channel_ = 1;
    float raw_x_     = 0.f;
    float raw_y_     = 0.f;
    float center_x_  = 0.5f;
    float center_y_  = 0.5f;
    bool  centered_  = false;

    // Scales so 0 -> -1, center -> 0, twice the center -> 1
    static float Center(float raw, float center)
    {
        if(center <= 0.f)
            return 0.f;
        float v = (raw - center) / center;
        if(v > 1.f)
            v = 1.f;
        if(v < -1.f)
            v = -1.f;
        if(v > -kDeadzone && v < kDeadzone)
            return 0.f;
        return v;
    }
};
