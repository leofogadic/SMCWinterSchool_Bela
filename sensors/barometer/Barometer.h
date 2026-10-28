#pragma once

#include <Bela.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>

// Adafruit BMP585 pressure / temperature sensor on Bela's I2C pins (SDA, SCL).
// Register map from the Bosch BMP5 Sensor API.
//
// Call Init() in setup() and Update() once per render(). The I2C reading
// happens in a background task (never in render(), which would cause
// audio dropouts), about every 20 ms. Read the result from temperature
// and pressure.
class Barometer
{
  public:
    // Updated in the background
    volatile float temperature = 0.f; // degrees C
    volatile float pressure    = 0.f; // Pa (divide by 100 for hPa)

    // Returns false if the sensor is not found.
    // bus: the I2C bus number (check with "i2cdetect -y -r 1" on the Bela)
    bool Init(int bus = 1)
    {
        char path[20];
        snprintf(path, sizeof(path), "/dev/i2c-%d", bus);
        fd_ = open(path, O_RDWR);
        if(fd_ < 0)
            return false;

        WriteReg(0x7E, 0xB6); // CMD: soft reset
        usleep(5000);

        uint8_t id = ReadReg(0x01); // CHIP_ID: 0x51 = BMP585, 0x50 = BMP581
        if(id != 0x51 && id != 0x50)
            return false;

        uint8_t status = ReadReg(0x28); // STATUS: NVM ready, no NVM error
        if(!(status & 0x02) || (status & 0x04))
            return false;

        WriteReg(0x36, 0x58); // OSR_CONFIG: pressure on, pressure 8x, temp 1x
        WriteReg(0x37, 0xBD); // ODR_CONFIG: 50 Hz, normal mode

        task_ = Bela_createAuxiliaryTask(ReadTask, 50, "barometer", this);
        return task_ != 0;
    }

    // Asks the background task for a new reading every ~20 ms
    // (the sensor measures 50 times per second)
    void Update(BelaContext* context)
    {
        if(task_ == 0)
            return;
        frames_ += context->audioFrames;
        if(frames_ >= context->audioSampleRate / 50)
        {
            frames_ = 0;
            Bela_scheduleAuxiliaryTask(task_);
        }
    }

  private:
    static const uint8_t kAddress = 0x47; // 0x46 if SDO is tied to GND

    int           fd_     = -1;
    AuxiliaryTask task_   = 0;
    unsigned int  frames_ = 0;

    // Runs in the background task
    static void ReadTask(void* data) { static_cast<Barometer*>(data)->Read(); }

    void Read()
    {
        uint8_t buf[6];
        if(!ReadRegs(0x1D, buf, 6))
            return;

        // 24-bit values, low byte first
        int32_t raw_t
            = (int32_t)((buf[2] << 24) | (buf[1] << 16) | (buf[0] << 8)) >> 8;
        uint32_t raw_p = (buf[5] << 16) | (buf[4] << 8) | buf[3];

        temperature = raw_t / 65536.f;
        pressure    = raw_p / 64.f;
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
