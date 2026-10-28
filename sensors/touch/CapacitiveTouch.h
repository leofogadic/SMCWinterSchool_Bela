#pragma once

#include <Bela.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>

// Adafruit MPR121 12-key capacitive touch sensor on Bela's I2C pins (SDA, SCL).
// Based on the Arduino Adafruit_MPR121 library.
//
// Call Init() in setup() and Update() once per render(), then use the
// functions below. The I2C reading happens in a background task (never in
// render(), which would cause audio dropouts), about every 5 ms.
// Pads are numbered 0 to 11.
class CapacitiveTouch
{
  public:
    static const int kNumPads = 12;

    // Returns false if the sensor is not found.
    // touch / release: how much a pad must change to count as touched /
    // released. Lower = more sensitive. Touch must be higher than release.
    // bus: the I2C bus number (check with "i2cdetect -y -r 1" on the Bela)
    bool Init(uint8_t touch = 12, uint8_t release = 6, int bus = 1)
    {
        char path[20];
        snprintf(path, sizeof(path), "/dev/i2c-%d", bus);
        fd_ = open(path, O_RDWR);
        if(fd_ < 0)
            return false;

        WriteReg(0x80, 0x63); // soft reset
        usleep(1000);
        WriteReg(0x5E, 0x00); // ECR: stop, so the settings below can be written

        if(ReadReg(0x5D) != 0x24) // CONFIG2 is 0x24 after a reset
            return false;

        // Touch and release thresholds for every pad
        for(uint8_t i = 0; i < kNumPads; i++)
        {
            WriteReg(0x41 + 2 * i, touch);
            WriteReg(0x42 + 2 * i, release);
        }

        // Baseline filtering (same values as the Adafruit library)
        WriteReg(0x2B, 0x01); // MHDR
        WriteReg(0x2C, 0x01); // NHDR
        WriteReg(0x2D, 0x0E); // NCLR
        WriteReg(0x2E, 0x00); // FDLR
        WriteReg(0x2F, 0x01); // MHDF
        WriteReg(0x30, 0x05); // NHDF
        WriteReg(0x31, 0x01); // NCLF
        WriteReg(0x32, 0x00); // FDLF
        WriteReg(0x33, 0x00); // NHDT
        WriteReg(0x34, 0x00); // NCLT
        WriteReg(0x35, 0x00); // FDLT

        WriteReg(0x5B, 0x00); // DEBOUNCE: none
        WriteReg(0x5C, 0x10); // CONFIG1: 16 uA charge current
        WriteReg(0x5D, 0x20); // CONFIG2: 0.5 us charge time, 1 ms sample period

        WriteReg(0x5E, 0x80 + kNumPads); // ECR: baseline tracking, all 12 pads on

        task_ = Bela_createAuxiliaryTask(ReadTask, 50, "touch", this);
        return task_ != 0;
    }

    // Takes the latest reading and asks the background task for a new one
    // every ~5 ms. Call once per render().
    void Update(BelaContext* context)
    {
        last_    = current_;
        current_ = touched_;

        if(task_ == 0)
            return;
        frames_ += context->audioFrames;
        if(frames_ >= context->audioSampleRate / 200)
        {
            frames_ = 0;
            Bela_scheduleAuxiliaryTask(task_);
        }
    }

    // True while pad is touched
    bool IsTouched(int pad) const { return current_ & (1 << pad); }

    // True only on the render() where pad was touched
    bool JustTouched(int pad) const
    {
        return (current_ & (1 << pad)) && !(last_ & (1 << pad));
    }

    // True only on the render() where pad was let go
    bool JustReleased(int pad) const
    {
        return !(current_ & (1 << pad)) && (last_ & (1 << pad));
    }

    // All 12 pads as bits: bit 0 = pad 0 ... bit 11 = pad 11
    uint16_t Touched() const { return current_; }

    // Raw reading of a pad (0 to 1023), lower when touched.
    // Useful for finding good thresholds or for "how close is the hand".
    uint16_t Raw(int pad) const { return raw_[pad]; }

  private:
    static const uint8_t kAddress = 0x5A; // ADDR pin unconnected = 0x5A

    int               fd_      = -1;
    AuxiliaryTask     task_    = 0;
    unsigned int      frames_  = 0;
    volatile uint16_t touched_ = 0; // written by the background task
    volatile uint16_t raw_[kNumPads] = {};
    uint16_t          current_ = 0;
    uint16_t          last_    = 0;

    // Runs in the background task
    static void ReadTask(void* data)
    {
        static_cast<CapacitiveTouch*>(data)->Read();
    }

    void Read()
    {
        // Registers 0x00-0x01: touch bits, 0x04-0x1B: raw value of each pad
        uint8_t buf[2 + 2 + kNumPads * 2];
        if(!ReadRegs(0x00, buf, sizeof(buf)))
            return;

        touched_ = ((buf[1] << 8) | buf[0]) & 0x0FFF;
        for(int i = 0; i < kNumPads; i++)
            raw_[i] = ((buf[5 + 2 * i] << 8) | buf[4 + 2 * i]) & 0x03FF;
    }

    // Write the register number, then read without a stop in between
    bool ReadRegs(uint8_t reg, uint8_t* buf, uint16_t len)
    {
        i2c_msg msgs[2] = {
            {kAddress, 0, 1, &reg},
            {kAddress, I2C_M_RD, len, buf},
        };
        i2c_rdwr_ioctl_data data = {msgs, 2};
        return ioctl(fd_, I2C_RDWR, &data) >= 0;
    }

    uint8_t ReadReg(uint8_t reg)
    {
        uint8_t value = 0;
        ReadRegs(reg, &value, 1);
        return value;
    }

    void WriteReg(uint8_t reg, uint8_t value)
    {
        uint8_t             bytes[2] = {reg, value};
        i2c_msg             msg      = {kAddress, 0, 2, bytes};
        i2c_rdwr_ioctl_data data     = {&msg, 1};
        ioctl(fd_, I2C_RDWR, &data);
    }
};
