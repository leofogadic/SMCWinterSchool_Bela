#pragma once

#include <Bela.h>

// HC-SR04 ultrasonic distance sensor on Bela digital pins (e.g. Trig D2, Echo D3).
// Power it from 3.3V: Bela's digital pins are not 5V tolerant.
//
// Call Init() in setup() and Read() once per render(). Read() never waits:
// it checks every digital frame of the block (~0.4 cm accuracy at 44.1 kHz)
// and returns the latest distance.
//
// Up to kMaxSensors can be used. They take turns (one pings at a time) so
// they don't hear each other's echoes. Each one updates about every 60 ms
// times the number of sensors.
class Ultrasonic
{
  public:
    static const int kMaxSensors = 4;

    void Init(BelaContext* context, int trig_pin, int echo_pin)
    {
        trig_ = trig_pin;
        echo_ = echo_pin;
        pinMode(context, 0, trig_, OUTPUT);
        pinMode(context, 0, echo_, INPUT);

        Shared& s = GetShared();
        if(s.count < kMaxSensors)
        {
            s.sensors[s.count] = this;
            s.count++;
        }
    }

    // Call once per render(). Returns the latest distance in cm,
    // or -1 if nothing was detected.
    float Read(BelaContext* context)
    {
        Shared& s = GetShared();

        // Only the sensor whose turn it is does any work
        if(s.count == 0 || s.sensors[s.active] != this)
            return distance_;

        for(unsigned int n = 0; n < context->digitalFrames; n++)
        {
            uint64_t now = FrameTime(context, n);
            if(Update(context, n, now))
            {
                // Done: hand over to the next sensor
                s.active = (s.active + 1) % s.count;
                s.sensors[s.active]->Go(State::IDLE, now);
                break;
            }
        }
        return distance_;
    }

  private:
    enum class State
    {
        IDLE,
        TRIGGER,
        WAIT_ECHO_HIGH,
        WAIT_ECHO_LOW,
    };

    // ~4 m range is ~23 ms of echo
    static constexpr float kTimeoutUs = 25000.f;
    // Pause before each ping so old echoes die out
    static constexpr float kGapUs = 30000.f;

    // Shared by all sensors: the list and whose turn it is
    struct Shared
    {
        Ultrasonic* sensors[kMaxSensors];
        int         count  = 0;
        int         active = 0;
    };

    static Shared& GetShared()
    {
        static Shared shared;
        return shared;
    }

    int      trig_        = 0;
    int      echo_        = 0;
    State    state_       = State::IDLE;
    uint64_t state_start_ = 0; // in digital frames
    float    distance_    = -1.f;

    // Digital frames since Bela started
    static uint64_t FrameTime(BelaContext* context, unsigned int n)
    {
        return context->audioFramesElapsed * context->digitalFrames
                   / context->audioFrames
               + n;
    }

    // Moves the measurement one frame forward. Returns true when finished.
    bool Update(BelaContext* context, unsigned int n, uint64_t now)
    {
        float elapsed
            = (now - state_start_) * 1000000.f / context->digitalSampleRate; // us

        switch(state_)
        {
            // Wait a bit, then start the measurement
            case State::IDLE:
                if(elapsed >= kGapUs)
                {
                    digitalWrite(context, n, trig_, HIGH);
                    Go(State::TRIGGER, now);
                }
                break;

            // Keep Trig high for at least 10 us (one frame is ~23 us)
            case State::TRIGGER:
                if(elapsed >= 10.f)
                {
                    digitalWrite(context, n, trig_, LOW);
                    Go(State::WAIT_ECHO_HIGH, now);
                }
                break;

            // Wait for Echo to go high
            case State::WAIT_ECHO_HIGH:
                if(digitalRead(context, n, echo_))
                    Go(State::WAIT_ECHO_LOW, now);
                else if(elapsed > kTimeoutUs)
                    return Finish(-1.f, now);
                break;

            // Echo stays high as long as the sound takes to come back
            case State::WAIT_ECHO_LOW:
                if(!digitalRead(context, n, echo_))
                    return Finish(elapsed / 58.f, now); // ~58 us per cm
                else if(elapsed > kTimeoutUs)
                    return Finish(-1.f, now);
                break;
        }
        return false;
    }

    void Go(State next, uint64_t now)
    {
        state_       = next;
        state_start_ = now;
    }

    bool Finish(float distance, uint64_t now)
    {
        distance_ = distance;
        Go(State::IDLE, now);
        return true;
    }
};
