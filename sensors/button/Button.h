#pragma once

#include <Bela.h>

// Reads a button on a Bela digital pin (e.g. D0).
// Default: external pull-down, button connects the pin to 3.3V when pressed.
// With pull_up = true: external pull-up, button connects the pin to GND.
//
// Call Init() in setup() and Update() once at the start of every render().
class Button
{
  public:
    void Init(BelaContext* context, int pin, bool pull_up = false)
    {
        pin_     = pin;
        pull_up_ = pull_up;
        pinMode(context, 0, pin_, INPUT);
    }

    // Reads the pin and filters out bounce. Call once every render().
    void Update(BelaContext* context)
    {
        uint32_t now = NowMs(context);
        bool     raw = digitalRead(context, 0, pin_);
        if(pull_up_)
            raw = !raw; // with a pull-up, pressed reads low

        just_pressed_  = false;
        just_released_ = false;

        // Restart the timer every time the raw pin changes
        if(raw != last_raw_)
        {
            last_raw_    = raw;
            last_change_ = now;
        }

        // Accept the new state once it has been stable for kDebounceMs
        if(now - last_change_ >= kDebounceMs && raw != pressed_)
        {
            pressed_ = raw;
            if(pressed_)
            {
                just_pressed_ = true;
                press_start_  = now;
                count_++;
            }
            else
            {
                just_released_ = true;
                last_held_ms_  = now - press_start_;
            }
        }
        now_ = now;
    }

    // True while the button is held down
    bool IsPressed() const { return pressed_; }

    // True only on the render() where the button went down
    bool JustPressed() const { return just_pressed_; }

    // True only on the render() where the button came up
    bool JustReleased() const { return just_released_; }

    // How long the button has been held, in ms (0 if not pressed)
    uint32_t HeldTime() const { return pressed_ ? now_ - press_start_ : 0; }

    // True while the button has been held for at least ms (long press)
    bool IsHeld(uint32_t ms) const { return HeldTime() >= ms; }

    // How long the last press lasted, in ms (valid after JustReleased())
    uint32_t LastPressTime() const { return last_held_ms_; }

    // Total number of presses since Init() or ResetCount()
    uint32_t Count() const { return count_; }
    void     ResetCount() { count_ = 0; }

    // Flips every time the button is pressed (on/off switch)
    bool Toggled() const { return count_ % 2 == 1; }

  private:
    static const uint32_t kDebounceMs = 10;

    int      pin_           = 0;
    bool     pull_up_       = false;
    bool     last_raw_      = false;
    bool     pressed_       = false;
    bool     just_pressed_  = false;
    bool     just_released_ = false;
    uint32_t now_           = 0;
    uint32_t last_change_   = 0;
    uint32_t press_start_   = 0;
    uint32_t last_held_ms_  = 0;
    uint32_t count_         = 0;

    // Time since Bela started, in ms
    static uint32_t NowMs(BelaContext* context)
    {
        return (uint32_t)(context->audioFramesElapsed * 1000
                          / (uint64_t)context->audioSampleRate);
    }
};