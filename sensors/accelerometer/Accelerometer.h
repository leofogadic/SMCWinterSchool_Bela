#pragma once

#include <Bela.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>

// Adafruit MMA8451 accelerometer on Bela's I2C pins (SDA, SCL).
// Based on the Arduino Adafruit_MMA8451 library. Range: +-2g, 14-bit.
//
// Call Init() in setup() and Update() once per render(). The I2C reading
// happens in a background task (never in render(), which would cause
// audio dropouts), about every 10 ms. Read the result from x, y, z.
class Accelerometer
{
  public:
    // Acceleration in m/s^2, updated in the background
    volatile float x = 0.f, y = 0.f, z = 0.f;

    // Returns false if the sensor is not found.
    // bus: the I2C bus number (check with "i2cdetect -y -r 1" on the Bela)
    bool Init(int bus = 1)
    {
        char path[20];
        snprintf(path, sizeof(path), "/dev/i2c-%d", bus);
        fd_ = open(path, O_RDWR);
        if(fd_ < 0)
            return false;

        if(ReadReg(0x0D) != 0x1A) // WHO_AM_I
            return false;

        WriteReg(0x2B, 0x40); // CTRL_REG2: reset
        usleep(10000);
        WriteReg(0x0E, 0x00); // XYZ_DATA_CFG: +-2g
        WriteReg(0x2B, 0x02); // CTRL_REG2: high resolution
        WriteReg(0x2A, 0x05); // CTRL_REG1: active, low noise

        task_ = Bela_createAuxiliaryTask(ReadTask, 50, "accelerometer", this);
        return task_ != 0;
    }

    // Asks the background task for a new reading every ~10 ms
    void Update(BelaContext* context)
    {
        if(task_ == 0)
            return;
        frames_ += context->audioFrames;
        if(frames_ >= context->audioSampleRate / 100)
        {
            frames_ = 0;
            Bela_scheduleAuxiliaryTask(task_);
        }
    }

  private:
    static const uint8_t kAddress = 0x1D; // A pin floating = 0x1D

    int           fd_     = -1;
    AuxiliaryTask task_   = 0;
    unsigned int  frames_ = 0;

    // Runs in the background task
    static void ReadTask(void* data)
    {
        static_cast<Accelerometer*>(data)->Read();
    }

    void Read()
    {
        uint8_t buf[6];
        if(!ReadRegs(0x01, buf, 6))
            return;

        // 14-bit values, left aligned in two bytes
        int16_t rx = (int16_t)((buf[0] << 8) | buf[1]) >> 2;
        int16_t ry = (int16_t)((buf[2] << 8) | buf[3]) >> 2;
        int16_t rz = (int16_t)((buf[4] << 8) | buf[5]) >> 2;

        // 4096 counts per g at +-2g
        x = rx / 4096.f * 9.80665f;
        y = ry / 4096.f * 9.80665f;
        z = rz / 4096.f * 9.80665f;
    }

    // Write the register number, then read without a stop in between
    // (the MMA8451 needs this "repeated start")
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
        uint8_t bytes[2]   = {reg, value};
        i2c_msg msg        = {kAddress, 0, 2, bytes};
        i2c_rdwr_ioctl_data data = {&msg, 1};
        ioctl(fd_, I2C_RDWR, &data);
    }
};
